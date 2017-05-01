#pragma once
#include <d2d1.h>

namespace AIZ
{
	class DPIScale
	{
	public:
		void Init(ID2D1Factory* pFactory)
		{
			FLOAT dpiX, dpiY;
			pFactory->GetDesktopDpi(&dpiX, &dpiY);
			dpiScaleReciprocalX = 96.0f / dpiX;
			dpiScaleReciprocalY = 96.0f / dpiY;
		}

		template<typename T> float PixelsToDipsX(T x)
		{
			return static_cast<float>(x)*dpiScaleReciprocalX;
		}
		template<typename T> float PixelsToDipsY(T y)
		{
			return static_cast<float>(y)*dpiScaleReciprocalY;
		}
		template<typename T> float DipsToPixelsX(T x)
		{
			return static_cast<float>(x)/dpiScaleReciprocalX;
		}
		template<typename T> float DipsToPixelsY(T y)
		{
			return static_cast<float>(y)/dpiScaleReciprocalY;
		}
	private:
		float dpiScaleReciprocalX = 1.0f;
		float dpiScaleReciprocalY = 1.0f;

	};

}