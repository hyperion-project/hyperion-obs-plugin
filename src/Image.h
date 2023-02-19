#pragma once

#include "ImageData.h"

// QT includes
#include <QExplicitlySharedDataPointer>

struct ColorRgba
{
	/// The red color channel
	uint8_t red;
	/// The green color channel
	uint8_t green;
	/// The blue color channel
	uint8_t blue;
	/// The alpha mask channel
	uint8_t alpha;
};

template <typename Pixel_T>
class Image
{
public:
	typedef Pixel_T pixel_type;

	///
	/// Default constructor for an image
	///
	Image() :
		_d_ptr(new ImageData<Pixel_T>(1, 1, Pixel_T()))
	{
	}

	///
	/// Constructor for an image with specified width and height
	///
	/// @param width The width of the image
	/// @param height The height of the image
	///
	Image(unsigned width, unsigned height) :
		_d_ptr(new ImageData<Pixel_T>(width, height, Pixel_T()))
	{
	}

	///
	/// Returns the width of the image
	///
	/// @return The width of the image
	///
	inline unsigned width() const
	{
		return _d_ptr->width();
	}

	///
	/// Returns the height of the image
	///
	/// @return The height of the image
	///
	inline unsigned height() const
	{
		return _d_ptr->height();
	}

	///
	/// Returns a memory pointer to the first pixel in the image
	/// @return The memory pointer to the first pixel
	///
	Pixel_T* memptr()
	{
		return _d_ptr->memptr();
	}

	///
	/// Returns a const memory pointer to the first pixel in the image
	/// @return The const memory pointer to the first pixel
	///
	const Pixel_T* memptr() const
	{
		return _d_ptr->memptr();
	}

	///
	/// Get size of buffer
	///
	ssize_t size() const
	{
		return _d_ptr->size();
	}

private:
	template<class T>
	friend class Image;

private:
	QExplicitlySharedDataPointer<ImageData<Pixel_T>> _d_ptr;
};

