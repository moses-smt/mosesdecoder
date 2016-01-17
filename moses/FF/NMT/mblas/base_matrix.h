#pragma once

namespace mblas {

class BaseMatrix {
  public:
    virtual size_t Rows() const = 0;
    virtual size_t Cols() const = 0;
};

}