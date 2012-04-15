#pragma once
#include "clUtilCommon.h"
#include "clUtilDeviceGroup.h"

namespace clUtil
{
  namespace Utility
  {
    struct IndexRange
    {
      size_t Start;
      size_t End;
    };

    struct DeviceStatus
    {
      double Time1;
      double Time2;
      IndexRange Range;
      cl_event WaitEvent;
      bool IsBusy;
      size_t DeviceID;

      DeviceStatus() : 
        Time1(0.0), 
        Time2(0.0), 
        Range(), 
        WaitEvent(NULL),
        IsBusy(false)
      {   
      }   
    }; 
  }

  class IScheduler
  {
    public:
      IScheduler() : mStart(0), mEnd(0) {}
      virtual Utility::IndexRange getWork(const size_t deviceGroup) = 0;
      virtual void updateModel(const Utility::DeviceStatus& status) = 0;
      virtual bool workRemains(size_t deviceGroup) const = 0;
      virtual void setRange(Utility::IndexRange& range)
      {
        mStart = range.Start;
        mEnd = range.End;
      }

    protected:
      size_t mStart;
      size_t mEnd;
  };

  class StaticScheduler : public IScheduler
  {
    public:
      StaticScheduler(size_t numChunks = 30) :
        IScheduler(),
        mNumChunks(numChunks),
        mChunkSize(0),
        mNextIteration(0)
      {
      }
      virtual Utility::IndexRange getWork(const size_t deviceGroup);
      virtual void updateModel(const Utility::DeviceStatus& status);
      virtual bool workRemains(size_t deviceGroup) const;
      virtual void setRange(Utility::IndexRange& range);

    private:
      size_t mNumChunks;
      size_t mChunkSize;
      size_t mNextIteration;
  };

  class EGSScheduler : public IScheduler
  {
    public:
      EGSScheduler();
      virtual Utility::IndexRange getWork(const size_t deviceGroup);
      virtual void updateModel(const Utility::DeviceStatus& status);
      virtual bool workRemains(size_t deviceGroup) const;
      virtual void setRange(Utility::IndexRange& range);

    private:
      std::vector<Utility::IndexRange> mTasks;
      std::vector<bool> mTasksValid;
      std::vector<size_t> mPerformanceRank;
      size_t mNextIteration;
  };

  //PINA Is Not Acronymic
  class PINAScheduler : public IScheduler
  {
    struct Sample
    {
      size_t Index;
      double Time;
      
      Sample() : Index(0), Time(0.0) {}
    };

    struct GroupTimingInfo
    {
      size_t IterationsCompleted;
      double MeanTime;

      GroupTimingInfo() : IterationsCompleted(0), MeanTime(0.0) {}
    };

    public:
      PINAScheduler(size_t numSamples = 5, size_t chunkSize = 512);
      virtual Utility::IndexRange getWork(const size_t deviceGroup);
      virtual void updateModel(const Utility::DeviceStatus& status);
      virtual bool workRemains(size_t deviceGroup) const;
      virtual void setRange(Utility::IndexRange& range);

    private:
      std::vector<Utility::IndexRange> mTasksRemaining;
      std::vector<size_t> mCurrentSample;
      std::vector<std::vector<Sample>> mModel;
      std::vector<GroupTimingInfo> mMeanIterationTime;
      size_t mIterationsRemaining;
      size_t mNumSamples;
      size_t mChunkSize;

      static double interpolate(const Sample& s0, 
                                const Sample& s1, 
                                size_t index);
  };
}