// Copyright 2008 David Talbot
//
// This file is part of RandLM
//
// RandLM is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RandLM is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RandLM.  If not, see <http://www.gnu.org/licenses/>.
#ifndef INC_RANDLM_FILTER_H
#define INC_RANDLM_FILTER_H

#include <cmath>
#include "FileHandler.h"
#include "util/check.hh"

#ifdef WIN32
#define log2(X) (log((double)X)/log((double)2))
#endif

namespace randlm
{

/* Class Filter wraps a contiguous array of data. Filter and its subclasses
  * implement read/write/increment functionality on arrays with arbitrary sized addresses
  * (i.e. an address may not use a full number of bytes). When converting to byte-based
  * representation we assume "unused" bits are to left.
  * E.g. if the underlying data is stored in units T = uint16 and the 'width' = 11
  * to read 'address' = 3 we extract bits at indices [33,42] (i.e. [11*3, 11*4 - 1])
  * and store in a uint16 in positions 0000 0001 111111 where the first 7 bits have
  * been masked out.
 */
template<typename T>
class Filter
{
public:
  Filter(uint64_t addresses, int width) : addresses_(addresses), width_(width), data_(NULL) {
    // number of bits in T
    cell_width_ = sizeof(T) << 3;
    // current implementation has following constraints
    CHECK(cell_width_ > 0 && cell_width_ <= 64 && cell_width_ >= width);
    // used for >> division
    log_cell_width_ = static_cast<int>(floor(log((double)cell_width_)/log((double)2) + 0.000001));
    // size of underlying data in Ts
    cells_ = ((addresses * width) + cell_width_ - 1) >> log_cell_width_;
    // instantiate underlying data
    data_ = new T[cells_];
    CHECK(data_ != NULL);
    CHECK(reset());
    // 'first_bit' marks the first bit used by 'address' (left padded with zeros).
    first_bit_ = (width % cell_width_ == 0) ? 0 : cell_width_ - (width % cell_width_);
    // mask for full cell
    full_mask_ = static_cast<T>(0xffffffffffffffffull);
    // mask for bits that make up the address
    address_mask_ = full_mask_ >> first_bit_;
  }
  Filter(FileHandler* fin, bool loaddata = true) : data_(NULL) {
    CHECK(loadHeader(fin));
    if (loaddata)
      CHECK(loadData(fin));
  }
  virtual ~Filter() {
    delete[] data_;
  }
  bool reset() {
    for (uint64_t i = 0; i < cells_; ++i)
      data_[i] = 0;
    return true;
  }
  count_t size() {
    // return approx size of filter in MBs
    return cells_ * sizeof(T) >> 20;
  }
  // read / write functions
  inline bool read(uint64_t address, T* value) {
    CHECK(address <= addresses_);
    // copy address to 'value'
    uint64_t data_bit = address * width_;
    uint32_t data_cell = (data_bit >> log_cell_width_); // % cells_;
    // 'offset' shows how address in 'data' and 'value' align
    int offset = (data_bit % cell_width_) - first_bit_;
    // they align so just copy across masking unneeded leading bits
    if (offset == 0) {
      *value = data_[data_cell] & address_mask_;
      return true;
    }
    // data address starts to left so shift it right
    if (offset < 0) {
      *value = (data_[data_cell] >> -offset) & address_mask_;
      return true;
    }
    // data address is to right so shift it left and look at one more cell to right
    *value = ((data_[data_cell] << offset)
              | (data_[data_cell + 1] >> (cell_width_ - offset))) & address_mask_ ;
    return true;
  }
  inline T read(uint64_t address) {
    CHECK(address <= addresses_);
    // return value at address
    T value = 0;
    uint64_t data_bit = address * width_;
    uint32_t data_cell = (data_bit >> log_cell_width_); // % cells_;
    // 'offset' shows how address in 'data' and 'value' align
    int offset = (data_bit % cell_width_) - first_bit_;
    // they align so just copy across masking unneeded leading bits
    if (offset == 0) {
      value = data_[data_cell] & address_mask_;
    }
    // data address starts to left so shift it right
    else if (offset < 0) {
      value = (data_[data_cell] >> -offset) & address_mask_;
    }
    // data address is to right so shift it left and look at one more cell to right
    else
      value = ((data_[data_cell] << offset)
               | (data_[data_cell + 1] >> (cell_width_ - offset))) & address_mask_ ;
    return value;
  }
  inline bool write(uint64_t address, T value) {
    CHECK(address <= addresses_);
    CHECK(log2(value) <= width_);
    // write 'value' to address
    uint64_t data_bit = address * width_;
    uint32_t data_cell = (data_bit >> log_cell_width_); // % cells_;
    // 'offset' shows how address in 'data' and 'value' align
    int offset = (data_bit % cell_width_) - first_bit_;
    // they align so just copy across masking unneeded leading zeros of value
    if (offset == 0) {
      data_[data_cell] = value | (data_[data_cell] & ~address_mask_);
      return true;
    }
    // address in data is to left so shift value left by -offset
    if (offset < 0) {
      data_[data_cell] = (value << -offset)
                         | (data_[data_cell] & ~(address_mask_ << -offset));
      return true;
    }
    // address in data is to right so shift value right by offset
    data_[data_cell] = (value >> offset) |
                       (data_[data_cell] & ~(address_mask_ >> offset));
    data_[data_cell + 1] = (value << (cell_width_ - offset)) |
                           (data_[data_cell + 1] & (full_mask_ >> offset));
    return true;
  }
  inline bool readWithFingerprint(uint64_t address, T finger, T* value) {
    // copy 'address' ^ 'finger' to 'value'
    uint64_t data_bit = address * width_;
    uint32_t data_cell = (data_bit >> log_cell_width_); // % cells_;
    // 'offset' shows how address in 'data' and 'value' align
    int offset = (data_bit % cell_width_) - first_bit_;
    // they align so just copy across masking unneeded leading bits
    if (offset == 0) {
      *value = (finger ^ data_[data_cell]) & address_mask_;
      return true;
    }
    // data address starts to left so shift it right
    if (offset < 0) {
      *value = ((data_[data_cell] >> -offset) ^ finger) & address_mask_;
      return true;
    }
    // data address is to right so shift it left and look at one more cell to right
    *value = (((data_[data_cell] << offset)
               | (data_[data_cell + 1] >> (cell_width_ - offset))) ^ finger)
             & address_mask_ ;
    return true;
  }
  inline bool writeWithFingerprint(uint64_t address, T finger, T value) {
    // write 'value' ^ 'finger' to address
    finger &= address_mask_; // make sure fingerprint is correct size
    uint64_t data_bit = address * width_;
    uint32_t data_cell = (data_bit >> log_cell_width_); // % cells_;
    // 'offset' shows how address in 'data' and 'value' align
    int offset = (data_bit % cell_width_) - first_bit_;
    // they align so just copy across masking unneeded leading zeros of value
    if (offset == 0) {
      data_[data_cell] = (finger ^ value) | (data_[data_cell] & ~address_mask_);
      return true;
    }
    // address in data is to left so shift value left by -offset
    if (offset < 0) {
      data_[data_cell] = ((finger ^ value) << -offset)
                         | (data_[data_cell] & ~(address_mask_ << -offset));
      return true;
    }
    // address in data is to right so shift value right by offset
    data_[data_cell] = ((finger ^ value) >> offset) |
                       (data_[data_cell] & ~(address_mask_ >> offset));
    data_[data_cell + 1] = ((finger ^ value) << (cell_width_ - offset)) |
                           (data_[data_cell + 1] & (full_mask_ >> offset));
    return true;
  }
  // debugging
  void printFilter(const std::string & prefix = "", uint32_t truncate = 64) {
    std::cout << prefix;
    for (uint32_t i = 0; i < cells_ && i < truncate; ++i) {
      for (int j = cell_width_ - 1; j >= 0; --j)
        if (data_[i] & (1ull << j))
          std::cout << 1;
        else
          std::cout << 0;
      std::cout << "\n";
    }
    std::cout << std::endl;
  }
  // i/o
  uint64_t getAddresses() {
    return addresses_;
  }
  int getWidth() {
    return width_;
  }
  int getCellWidth() {
    return cell_width_;
  }
  uint32_t getCells() {
    return cells_;
  }
  virtual bool save(FileHandler* out) {
    CHECK(out != NULL);
    CHECK(out->write((char*)&cells_, sizeof(cells_)));
    CHECK(out->write((char*)&cell_width_, sizeof(cell_width_)));
    CHECK(out->write((char*)&log_cell_width_, sizeof(log_cell_width_)));
    CHECK(out->write((char*)&addresses_, sizeof(addresses_)));
    CHECK(out->write((char*)&width_, sizeof(width_)));
    CHECK(out->write((char*)&first_bit_, sizeof(first_bit_)));
    CHECK(out->write((char*)&full_mask_, sizeof(full_mask_)));
    CHECK(out->write((char*)&address_mask_, sizeof(address_mask_)));
    //CHECK(out->write((char*)data_, cells_ * sizeof(T)));
    const uint64_t jump  =  524288032ul; //(uint64_t)pow(2, 29);
    if((width_ == 1) || cells_ < jump)
      CHECK(out->write((char*)data_, cells_ * sizeof(T)));
    else {
      uint64_t idx(0);
      while(idx + jump < cells_) {
        CHECK(out->write((char*)&data_[idx], jump * sizeof(T)));
        idx += jump;
      }
      CHECK(out->write((char*)&data_[idx], (cells_ - idx) * sizeof(T)));
    }
    return true;
  }
protected:
  bool loadHeader(FileHandler* fin) {
    CHECK(fin != NULL);
    CHECK(fin->read((char*)&cells_, sizeof(cells_)));
    CHECK(fin->read((char*)&cell_width_, sizeof(cell_width_)));
    CHECK(cell_width_ == sizeof(T) << 3); // make sure correct underlying data type
    CHECK(fin->read((char*)&log_cell_width_, sizeof(log_cell_width_)));
    CHECK(fin->read((char*)&addresses_, sizeof(addresses_)));
    CHECK(fin->read((char*)&width_, sizeof(width_)));
    CHECK(fin->read((char*)&first_bit_, sizeof(first_bit_)));
    CHECK(fin->read((char*)&full_mask_, sizeof(full_mask_)));
    CHECK(fin->read((char*)&address_mask_, sizeof(address_mask_)));
    return true;
  }
  bool loadData(FileHandler* fin) {
    // instantiate underlying array
    data_ = new T[cells_];
    CHECK(data_ != NULL);
    CHECK(fin->read((char*)data_, cells_ * sizeof(T)));
    //CHECK(fin->read((char*)&data_[0], ceil(float(cells_) / 2.0) * sizeof(T)));
    //CHECK(fin->read((char*)&data_[cells_ / 2], (cells_ / 2) * sizeof(T)));
    return true;
  }
  uint64_t cells_;  // number T making up 'data_'
  int cell_width_;  // bits per cell (i.e. sizeof(T) << 3)
  int log_cell_width_;  // log of bits used for >> division
  uint64_t addresses_;  // number of addresses in the filter
  int width_;  // width in bits of each address
  int first_bit_;  // position of first bit in initial byte
  T full_mask_;  // all 1s
  T address_mask_;  // 1s in those positions that are part of address
  T* data_;  // the raw data as bytes
};

// Extension with bit test/setter methods added
class BitFilter : public Filter<uint8_t>
{
public:
  BitFilter(uint64_t bits) : Filter<uint8_t>(bits, 1) {}
  BitFilter(FileHandler* fin, bool loaddata = true)
    : Filter<uint8_t>(fin, loaddata) {
    if (loaddata)
      CHECK(load(fin));
  }
  // TODO: overload operator[]
  virtual bool testBit(uint64_t location) {
    // test bit referenced by location
    return data_[(location % addresses_) >> 3] & 1 << ((location % addresses_) % 8);
  }
  virtual bool setBit(uint64_t location) {
    // set bit referenced by location
    data_[(location % addresses_) >> 3] |= 1 << ((location % addresses_) % 8);
    return true;
  }
  virtual bool clearBit(uint64_t location) {
    // set bit referenced by location
    data_[(location % addresses_) >> 3] &= 0 << ((location % addresses_) % 8);
    return true;
  }
  bool save(FileHandler* fout) {
    CHECK(Filter<uint8_t>::save(fout));
    std::cerr << "Saved BitFilter. Rho = " << rho() << "." << std::endl;;
    return true;
  }
  float rho(uint64_t limit = 0) {
    uint64_t ones = 0;
    uint64_t range = limit > 0 ? std::min(limit,cells_) : cells_;
    for (uint64_t i = 0; i < range; ++i)
      for (int j = 0; j < 8; ++j)
        if (data_[i] & (1 << j))
          ++ones;
    return static_cast<float>((range << 3) - ones)/static_cast<float>(range << 3);
  }
protected:
  bool load(FileHandler* fin) {
    std::cerr << "Loaded BitFilter. Rho = " << rho() << "." << std::endl;;
    return true;
  }
};
/*
  // ResizedBitFilter deals with resizing to save memory
  // whereas other filters should expect locations to be within range
  // this filter will need to resize (and possibly rehash) locations
  // to fit a smaller range.
  class ResizedBitFilter : public BitFilter {
  public:
    ResizedBitFilter(FileHandler* fin) : BitFilter(fin) {
      CHECK(load(fin));
    }
    ResizedBitFilter(FileHandler* fin, uint64_t newsize) : BitFilter(newsize) {
      CHECK(resizeFromFile(fin, newsize));
    }
    bool resizeFromFile(FileHandler* oldin, uint64_t newsize);
    virtual bool testBit(uint64_t location) {
      // test bit referenced by location
      return BitFilter::testBit((location % old_addresses_) * a_ + b_);
    }
    virtual bool setBit(uint64_t location) {
      // set bit referenced by location
      return BitFilter::setBit((location % old_addresses_) * a_ + b_);
    }
    bool save(FileHandler* fout) {
      // re-hashing parameters
      CHECK(BitFilter::save(fout));
      std::cerr << "Saved ResizedBitFilter. Rho = " << rho() << "." << std::endl;
      CHECK(fout->write((char*)&old_addresses_, sizeof(old_addresses_)));
      CHECK(fout->write((char*)&a_, sizeof(a_)));
      return fout->write((char*)&b_, sizeof(b_));
    }
  protected:
    bool load(FileHandler* fin) {
      // re-hashing parameters
      std::cerr << "Loaded ResizedBitFilter. Rho = " << rho() << "." << std::endl;
      CHECK(fin->read((char*)&old_addresses_, sizeof(old_addresses_)));
      CHECK(fin->read((char*)&a_, sizeof(a_)));
      return fin->read((char*)&b_, sizeof(b_));
    }
    // member data
    uint64_t old_addresses_;  // size of pre-resized filter
    uint64_t a_, b_;  // re-hashing parameters (needed?)
  };

  // CountingFilter supports increment operator. Addresses
  // of the filter are treated as counters that store their counts
  // in big-endian format (i.e. leftmost bit is most significant).
  template<typename T>
    class CountingFilter : public Filter<T> {
    public:
    CountingFilter(uint64_t addresses, int width, bool wrap_around = true) :
      Filter<T>(addresses, width), wrap_around_(wrap_around) {}
    CountingFilter(FileHandler* fin) : Filter<T>(fin, true) {
      CHECK(load(fin));
    }
    ~CountingFilter() {}
    // increment this address by one
    inline bool increment(uint32_t address) {
      uint64_t data_bit = address * this->width_;  // index of first bit
      uint32_t data_cell = (data_bit >> this->log_cell_width_); // % this->cells_;  // index in data_
      // 'offset' shows how address in 'data' and 'value' align
      data_bit %= this->cell_width_;
      int offset = data_bit - this->first_bit_;
      // start from right incrementing and carrying if necessary
      bool carry = true;
      if (offset > 0) { // counter spans two cells
	carry = incrementSubCell(0, offset, &this->data_[data_cell + 1]);
	if (carry)
	  carry = incrementSubCell(data_bit, this->width_ - offset, &this->data_[data_cell]);
      } else { // counter is within a single cell
	carry = incrementSubCell(data_bit, this->width_, &this->data_[data_cell]);
      }
      // last update must not have carried
      if (!carry)
	return true;
      // wrapped round so check whether need to reset to max count
      if (!wrap_around_)
	CHECK(this->write(address, this->address_mask_));
      return false; // false to indicate that overflowed
    }
    bool save(FileHandler* fout) {
      CHECK(Filter<T>::save(fout));
      return fout->write((char*)&wrap_around_, sizeof(wrap_around_));
    }
    private:
    bool load(FileHandler* fin) {
      return fin->read((char*)&wrap_around_, sizeof(wrap_around_));
    }
    inline bool incrementSubCell(int bit, int len, T* cell) {
      // increment counter consisting of bits [startbit, startbit + len - 1] rest stays unchanged
      *cell = ((((*cell >> (this->cell_width_ - bit - len)) + 1)
		& (this->full_mask_ >> (this->cell_width_ - len))) << (this->cell_width_ - bit - len))
	| (*cell & ~(((this->full_mask_ >> (this->cell_width_ - len)) << (this->cell_width_ - bit - len))));
      // indicate overflow as true
      return  ((*cell & (this->full_mask_ >> bit)) >> (this->cell_width_ - bit - len)) == 0;
    }
    bool wrap_around_;   // whether to start from 0 on overflow (if not just stay at maximum count)
  };
*/
}
#endif // INC_RANDLM_FILTER_H
