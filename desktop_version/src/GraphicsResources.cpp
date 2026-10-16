#include "GraphicsResources.h"
#include "FileSystemUtils.h"
#include <stdio.h>
#include <stdlib.h>

// Used to load PNG data
extern "C"
{
	extern unsigned lodepng_decode24(
		unsigned char** out,
		unsigned* w,
		unsigned* h,
		const unsigned char* in,
		size_t insize
	);
	extern unsigned lodepng_decode32(
		unsigned char** out,
		unsigned* w,
		unsigned* h,
		const unsigned char* in,
		size_t insize
	);
	extern const char *lodepng_error_text(unsigned code);
}

SDL_Surface* LoadImage(const char *filename, bool noBlend = true, bool noAlpha = false)
{
	//Temporary storage for the image that's loaded
	SDL_Surface* loadedImage = NULL;
	//The optimized image that will be used
	SDL_Surface* optimizedImage = NULL;

	unsigned char *data = NULL;
	unsigned int width = 0, height = 0;

	unsigned char *fileIn = NULL;
	size_t length = 0;
	FILESYSTEM_loadFileToMemory(filename, &fileIn, &length);
	if (!fileIn)
	{
		fprintf(stderr, "Could not load image: %s\n", filename);
		return NULL;
	}
	if (noAlpha)
	{
		unsigned error = lodepng_decode24(&data, &width, &height, fileIn, length);
		if (error)
		{
			fprintf(stderr, "Could not decode image %s: %s\n", filename, lodepng_error_text(error));
			free(data);
			free(fileIn);
			return NULL;
		}
	}
	else
	{
		unsigned error = lodepng_decode32(&data, &width, &height, fileIn, length);
		if (error)
		{
			fprintf(stderr, "Could not decode image %s: %s\n", filename, lodepng_error_text(error));
			free(data);
			free(fileIn);
			return NULL;
		}
	}
	FILESYSTEM_freeMemory(&fileIn);

	loadedImage = SDL_CreateRGBSurfaceFrom(
		data,
		width,
		height,
		noAlpha ? 24 : 32,
		width * (noAlpha ? 3 : 4),
		0x000000FF,
		0x0000FF00,
		0x00FF0000,
		noAlpha ? 0x00000000 : 0xFF000000
	);

	if (loadedImage != NULL)
	{
		optimizedImage = SDL_ConvertSurfaceFormat(
			loadedImage,
			SDL_PIXELFORMAT_ABGR8888, // FIXME: Format? -flibit
			0
		);
		SDL_FreeSurface( loadedImage );
		free(data);
		if (noBlend)
		{
			SDL_SetSurfaceBlendMode(optimizedImage, SDL_BLENDMODE_BLEND);
		}
		return optimizedImage;
	}
	else
	{
		fprintf(stderr,"Image not found: %s\n", filename);
		SDL_assert(0 && "Image not found! See stderr.");
		return NULL;
	}
}

GraphicsResources::GraphicsResources(void)
{
	im_tiles =		LoadImage("graphics/tiles.png");
	im_tiles2 =		LoadImage("graphics/tiles2.png");
	im_tiles3 =		LoadImage("graphics/tiles3.png");
	im_entcolours =		LoadImage("graphics/entcolours.png");
	im_sprites =		LoadImage("graphics/sprites.png");
	im_flipsprites =	LoadImage("graphics/flipsprites.png");
	im_bfont =		LoadImage("graphics/font.png");
	im_bfontmask =		LoadImage("graphics/fontmask.png");
	im_teleporter =		LoadImage("graphics/teleporter.png");

	im_image0 =		LoadImage("graphics/levelcomplete.png", false);
	im_image1 =		LoadImage("graphics/minimap.png", true, true);
	im_image2 =		LoadImage("graphics/covered.png", true, true);
	im_image3 =		LoadImage("graphics/elephant.png");
	im_image4 =		LoadImage("graphics/gamecomplete.png", false);
	im_image5 =		LoadImage("graphics/fliplevelcomplete.png", false);
	im_image6 =		LoadImage("graphics/flipgamecomplete.png", false);
	im_image7 =		LoadImage("graphics/site.png", false);
	im_image8 =		LoadImage("graphics/site2.png");
	im_image9 =		LoadImage("graphics/site3.png");
	im_image10 =		LoadImage("graphics/ending.png");
	im_image11 =		LoadImage("graphics/site4.png");
	im_image12 =		LoadImage("graphics/minimap.png");
}


GraphicsResources::~GraphicsResources(void)
{
	SDL_Surface *surfaces[] = {im_tiles, im_tiles2, im_tiles3, im_entcolours,
		im_sprites, im_flipsprites, im_bfont, im_bfontmask, im_teleporter,
		im_image0, im_image1, im_image2, im_image3, im_image4, im_image5,
		im_image6, im_image7, im_image8, im_image9, im_image10, im_image11,
		im_image12};
	for (SDL_Surface *surface : surfaces) SDL_FreeSurface(surface);
}
