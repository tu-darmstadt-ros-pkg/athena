#ifndef ATHENA_MOTOR_FIRMWARE_FILTERS_MEAN_FILTER_H
#define ATHENA_MOTOR_FIRMWARE_FILTERS_MEAN_FILTER_H

#include <array>

template<typename T, int COUNT>
class MeanFilter
{
public:
  MeanFilter() { reset(); }

  void addValue( T value );

  T getMean() const;

  T getSum() const { return sum_; }

  int count() const { return full_ ? COUNT : index_; }

  void reset()
  {
    values_.fill( 0 );
    index_ = 0;
    full_ = false;
    sum_ = 0;
    mean_ = 0;
  }

private:
  std::array<T, COUNT> values_ = {};
  int index_ = 0;
  bool full_ = false;
  T sum_ = 0;
  T mean_ = 0;
};

template<typename T, int COUNT>
void MeanFilter<T, COUNT>::addValue( T value )
{
  sum_ -= values_[index_];
  values_[index_] = value;
  sum_ += value;
  if ( ++index_ == COUNT ) {
    full_ = true;
    index_ = 0;
    if ( std::is_floating_point<T>::value ) {
      // Recompute mean if T is a floating point type, to avoid precision errors accumulating
      sum_ = 0;
      for ( int i = 0; i < COUNT; ++i ) { sum_ += values_[i]; }
    }
  }
}

template<typename T, int COUNT>
T MeanFilter<T, COUNT>::getMean() const
{
  if ( !full_ ) {
    return sum_ / index_;
  }
  return sum_ / COUNT;
}

#endif // ATHENA_MOTOR_FIRMWARE_FILTERS_MEAN_FILTER_H