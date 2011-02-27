#pragma once
#include "kml_parser.hpp"
#include "world_map_generator.hpp"
#include "../../indexer/feature.hpp"
#include "../../indexer/feature_visibility.hpp"
#include "../../indexer/cell_id.hpp"
#include "../../geometry/rect2d.hpp"
#include "../../coding/file_writer.hpp"
#include "../../base/base.hpp"
#include "../../base/buffer_vector.hpp"
#include "../../base/macros.hpp"
#include "../../std/scoped_ptr.hpp"
#include "../../std/string.hpp"

#ifndef PARALLEL_POLYGONIZER
#define PARALLEL_POLYGONIZER 1
#endif

#if PARALLEL_POLYGONIZER
#include <QSemaphore>
#include <QThreadPool>
#include <QMutex>
#include <QMutexLocker>
#endif

namespace feature
{
  // Groups features according to country polygons
  template <class FeatureOutT, class BoundsT, typename CellIdT>
  class Polygonizer
  {
  public:
    template <class TInfo>
    Polygonizer(TInfo & info) : m_FeatureOutInitData(info.datFilePrefix, info.datFileSuffix),
      m_worldMap(info.maxScaleForWorldFeatures, info.mergeCoastlines, m_FeatureOutInitData)
#if PARALLEL_POLYGONIZER
    , m_ThreadPoolSemaphore(m_ThreadPool.maxThreadCount() * 4)
#endif
    {
      CHECK(kml::LoadCountriesList(info.datFilePrefix, m_countries, info.simplifyCountriesLevel),
            ("Error loading country polygons files"));

      //LOG_SHORT(LINFO, ("Loaded polygons count for regions:"));
      //for (size_t i = 0; i < m_countries.size(); ++i)
      //{
      //  LOG_SHORT(LINFO, (m_countries[i].m_name, m_countries[i].m_regions.size()));
      //}
    }
    ~Polygonizer()
    {
      Finish();
      for_each(m_Buckets.begin(), m_Buckets.end(), DeleteFunctor());
    }

    struct PointChecker
    {
      kml::RegionsContainerT const & m_regions;
      bool m_belongs;

      PointChecker(kml::RegionsContainerT const & regions)
        : m_regions(regions), m_belongs(false) {}

      bool operator()(m2::PointD const & pt)
      {
        m_regions.ForEachInRect(m2::RectD(pt, pt), bind<void>(ref(*this), _1, cref(pt)));
        return !m_belongs;
      }

      void operator() (kml::Region const & rgn, kml::Region::value_type const & point)
      {
        if (!m_belongs)
          m_belongs = rgn.Contains(point);
      }
    };

    class InsertCountriesPtr
    {
      typedef buffer_vector<kml::CountryPolygons const *, 32> vec_type;
      vec_type & m_vec;

    public:
      InsertCountriesPtr(vec_type & vec) : m_vec(vec) {}
      void operator() (kml::CountryPolygons const & c)
      {
        m_vec.push_back(&c);
      }
    };

    void operator () (FeatureBuilder1 const & fb)
    {
      if (m_worldMap(fb))
        return; // do not duplicate feature in any country if it's stored in world map

      buffer_vector<kml::CountryPolygons const *, 32> vec;
      m_countries.ForEachInRect(fb.GetLimitRect(), InsertCountriesPtr(vec));

      if (vec.size() == 1)
        EmitFeature(vec[0], fb);
      else
      {
#if PARALLEL_POLYGONIZER
        m_ThreadPoolSemaphore.acquire();
        m_ThreadPool.start(new PolygonizerTask(this, vec, fb));
#else
        PolygonizerTask task(this, vec, fb);
        task.run();
#endif
      }
    }

    void Finish()
    {
#if PARALLEL_POLYGONIZER
      m_ThreadPool.waitForDone();
#endif
    }

    void EmitFeature(kml::CountryPolygons const * country, FeatureBuilder1 const & fb)
    {
#if PARALLEL_POLYGONIZER
      QMutexLocker mutexLocker(&m_EmitFeatureMutex);
      UNUSED_VALUE(mutexLocker);
#endif
      if (country->m_index == -1)
      {
        m_Names.push_back(country->m_name);
        m_Buckets.push_back(new FeatureOutT(country->m_name, m_FeatureOutInitData));
        country->m_index = m_Buckets.size()-1;
      }

      (*(m_Buckets[country->m_index]))(fb);
    }

    vector<string> const & Names()
    {
      return m_Names;
    }

  private:
    typename FeatureOutT::InitDataType m_FeatureOutInitData;

    vector<FeatureOutT*> m_Buckets;
    vector<string> m_Names;
    kml::CountriesContainerT m_countries;
    WorldMapGenerator<FeatureOutT> m_worldMap;

#if PARALLEL_POLYGONIZER
    QThreadPool m_ThreadPool;
    QSemaphore m_ThreadPoolSemaphore;
    QMutex m_EmitFeatureMutex;

    friend class PolygonizerTask;

    class PolygonizerTask : public QRunnable
    {
    public:
      PolygonizerTask(Polygonizer * pPolygonizer,
                      buffer_vector<kml::CountryPolygons const *, 32> const & countries,
                      FeatureBuilder1 const & fb)
        : m_pPolygonizer(pPolygonizer), m_Countries(countries), m_FB(fb) {}

      void run()
      {
        for (size_t i = 0; i < m_Countries.size(); ++i)
        {
          PointChecker doCheck(m_Countries[i]->m_regions);
          m_FB.ForEachTruePointRef(doCheck);

          if (doCheck.m_belongs)
            m_pPolygonizer->EmitFeature(m_Countries[i], m_FB);
        }

        m_pPolygonizer->m_ThreadPoolSemaphore.release();
      }

    private:
      Polygonizer * m_pPolygonizer;
      buffer_vector<kml::CountryPolygons const *, 32> m_Countries;
      FeatureBuilder1 m_FB;
    };
#endif
  };
}
