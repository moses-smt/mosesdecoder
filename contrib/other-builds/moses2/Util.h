#pragma once

template<typename T>
void Init(T arr[], size_t size, const T &val) {
	for (size_t i = 0; i < size; ++i) {
		arr[i] = val;
	}
}

template<typename T>
void Swap(T &a, T &b) {
  T &tmp = a;
  a = b;
  b = tmp;
}
