#pragma once

// STL includes
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <type_traits>

// QT includes
#include <QSharedData>

// https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types#ssize-t
#if defined(_MSC_VER)
	#include <BaseTsd.h>
	typedef SSIZE_T ssize_t;
#endif

template <typename Pixel_T>
class ImageData : public QSharedData
{
public:
	typedef Pixel_T pixel_type;

	ImageData(unsigned width, unsigned height, const Pixel_T background) :
		_width(width),
		_height(height),
		_pixels(new Pixel_T[width * height + 1])
	{
		std::fill(_pixels, _pixels + width * height, background);
	}

	~ImageData()
	{
		delete[] _pixels;
	}

	inline unsigned width() const
	{
		return _width;
	}

	inline unsigned height() const
	{
		return _height;
	}

	Pixel_T* memptr()
	{
		return _pixels;
	}

	const Pixel_T* memptr() const
	{
		return _pixels;
	}

	ssize_t size() const
	{
		return  static_cast<ssize_t>(_width) * static_cast<ssize_t>(_height) * sizeof(Pixel_T);
	}

private:
	/// The width of the image
	unsigned _width;
	/// The height of the image
	unsigned _height;
	/// The pixels of the image
	Pixel_T* _pixels;
};
