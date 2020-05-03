#include "loaders.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <png.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "bridge.hpp"

// Must have already bound texture
bool loadGLImage(const char* name, GLenum target) {
	int width, height, nrChannels;
	unsigned char *data = stbi_load((getResourcesPath() + "/textures/" + name).c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	
	else {
		std::cout << "Failed to load texture " << name << std::endl;
	}
	stbi_image_free(data);
	
	return (bool) data;
}

GLuint loadTexture(const char* name) {
	
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load and generate the texture
	loadGLImage(name, GL_TEXTURE_2D);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	return texture;
}

GLuint loadCubemap(const std::vector<std::string>& faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    for (unsigned int i = 0; i < faces.size(); i++) {
		loadGLImage(faces[i].c_str(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

std::string loadShaderFile(const char* name) {
	FILE* file = fopen((getResourcesPath() + "/shaders/" + name).c_str(), "r");
	if (file == nullptr) {
		perror((std::string("Error loading shader ") + name).c_str());
		glfwTerminate();
		exit(1);
	}
	
	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET);  /* same as rewind(f); */

	std::string shaderString(fsize, '\0');
	fread(&shaderString.front(), 1, fsize, file);
	fclose(file);
	return shaderString;
}

// Processes includes. Must be of the form @include "file" as the first thing on a line
GLuint loadShader(const char* name, GLenum type) {
	
	std::string shaderString = loadShaderFile(name);
	
	// Very quick and dirty include handling
	std::vector<std::string> includeFiles;
	std::vector<const char*> stringPtrs(1, shaderString.c_str());
	
	size_t lastPos = 0;
	while ((lastPos = shaderString.find("\n@include", lastPos)) != std::string::npos) {
		
		size_t lineEnd = shaderString.find("\n", lastPos + 1);
		std::string line = shaderString.substr(lastPos, lineEnd - lastPos);
		size_t quotedStart = line.find("\"") + 1;
		std::string quoted = line.substr(quotedStart, line.rfind("\"") - quotedStart);
		
		if (quoted.length() < 1) {
			std::cout << "Shader include error: not enough quotes, or nothing between them" << std::endl;
		}
		
		includeFiles.push_back(loadShaderFile(quoted.c_str()));
		stringPtrs.push_back(includeFiles.back().c_str());
		shaderString[lastPos] = '\0';
		stringPtrs.push_back(&shaderString[lineEnd]);
	}
	
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, stringPtrs.size(), stringPtrs.data(), NULL);
	glCompileShader(shader);
	
	int  success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	
	if(!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << "ERROR: SHADER " << name << " COMPILATION FAILED\n" << infoLog << std::endl;
		glfwTerminate();
		exit(1);
	}
	return shader;
}

GLuint linkShaders(std::vector<GLuint> shaders,  bool deleteShaders, std::function<void(GLuint)> preLinkOptions) {
	
	GLuint shaderProgram = glCreateProgram();
	for (GLuint i : shaders) {
		glAttachShader(shaderProgram, i);
	}
	if (preLinkOptions != nullptr) preLinkOptions(shaderProgram);
	
	glLinkProgram(shaderProgram);
	
	int success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR: SHADER LINKING FAILED\n" << infoLog << std::endl;
		glfwTerminate();
		exit(1);
	}
	
	if (deleteShaders) for (GLuint i : shaders) {
		glDeleteShader(i);
	}
	
	return shaderProgram;
}

GLuint loadShaders(const char* vert, const char* frag) {
	
	GLuint vertexShader = loadShader(vert, GL_VERTEX_SHADER);
	GLuint fragmentShader = loadShader(frag, GL_FRAGMENT_SHADER);
	
	return linkShaders({vertexShader, fragmentShader});
}

GLenum glCheckError_(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            //case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            //case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}


// From libpng/example.c
// Modified to only do RGBA images
void write_png(const char *file_name, uint8_t* imageData, int width, int height)
{
	
   FILE *fp;
   png_structp png_ptr;
   png_infop info_ptr;
   png_colorp palette;

   /* Open the file */
   fp = fopen(file_name, "wb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file %s", file_name);
		return;
	}

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (png_ptr == NULL) {
	   fclose(fp);
	   fprintf(stderr, "Could not initialize libpng");
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  NULL);
      fprintf(stderr, "Could not initialize libpng");
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error handling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_jmpbuf(png_ptr)))
   {
      /* If we get here, we had a problem writing the file */
      fclose(fp);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      fprintf(stderr, "Could not initialize libpng");
   }

   /* One of the following I/O initialization functions is REQUIRED */


   /* Set up the output control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* This is the hard way */

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
    */
   png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   /* Set the palette if there is one.  REQUIRED for indexed-color images */
  // palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH
   //          * png_sizeof(png_color));
   /* ... Set palette colors ... */
   //png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);
   /* You must not free palette here, because png_set_PLTE only makes a link to
    * the palette that you malloced.  Wait until you are about to destroy
    * the png structure.
    */

	/*
   // Optional significant bit (sBIT) chunk
   png_color_8 sig_bit;

   //If we are dealing with a grayscale image then
   sig_bit.gray = true_bit_depth;

   // Otherwise, if we are dealing with a color image then
   sig_bit.red = true_red_bit_depth;
   sig_bit.green = true_green_bit_depth;
   sig_bit.blue = true_blue_bit_depth;

   // If the image has an alpha channel then
   sig_bit.alpha = true_alpha_bit_depth;

   png_set_sBIT(png_ptr, info_ptr, &sig_bit);
*/

   /* Optional gamma chunk is strongly suggested if you have any guess
    * as to the correct gamma of the image.
    */
  // png_set_gAMA(png_ptr, info_ptr, gamma);

   /* Optionally write comments into the image */
   /*{
      png_text text_ptr[3];

      char key0[]="Title";
      char text0[]="Mona Lisa";
      text_ptr[0].key = key0;
      text_ptr[0].text = text0;
      text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
      text_ptr[0].itxt_length = 0;
      text_ptr[0].lang = NULL;
      text_ptr[0].lang_key = NULL;

      char key1[]="Author";
      char text1[]="Leonardo DaVinci";
      text_ptr[1].key = key1;
      text_ptr[1].text = text1;
      text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
      text_ptr[1].itxt_length = 0;
      text_ptr[1].lang = NULL;
      text_ptr[1].lang_key = NULL;

      char key2[]="Description";
      char text2[]="<long text>";
      text_ptr[2].key = key2;
      text_ptr[2].text = text2;
      text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
      text_ptr[2].itxt_length = 0;
      text_ptr[2].lang = NULL;
      text_ptr[2].lang_key = NULL;

      png_set_text(write_ptr, write_info_ptr, text_ptr, 3);
   }*/

   /* Other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs */

   /* Note that if sRGB is present the gAMA and cHRM chunks must be ignored
    * on read and, if your application chooses to write them, they must
    * be written in accordance with the sRGB profile
    */

   /* Write the file header information.  REQUIRED */
   png_write_info(png_ptr, info_ptr);

   /* If you want, you can write the info in two steps, in case you need to
    * write your private chunk ahead of PLTE:
    *
    *   png_write_info_before_PLTE(write_ptr, write_info_ptr);
    *   write_my_chunk();
    *   png_write_info(png_ptr, info_ptr);
    *
    * However, given the level of known- and unknown-chunk support in 1.2.0
    * and up, this should no longer be necessary.
    */

   /* Once we write out the header, the compression type on the text
    * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
    * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
    * at the end.
    */

   /* Set up the transformations you want.  Note that these are
    * all optional.  Only call them if you want them.
    */

   /* Invert monochrome pixels */
   //png_set_invert_mono(png_ptr);

   /* Shift the pixels up to a legal bit depth and fill in
    * as appropriate to correctly scale the image.
    */
   //png_set_shift(png_ptr, &sig_bit);

   /* Pack pixels into bytes */
   //png_set_packing(png_ptr);

   /* Swap location of alpha bytes from ARGB to RGBA */
   //png_set_swap_alpha(png_ptr);

   /* Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
    * RGB (4 channels -> 3 channels). The second parameter is not used.
    */
   //png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

   /* Flip BGR pixels to RGB */
   //png_set_bgr(png_ptr);

   /* Swap bytes of 16-bit files to most significant byte first */
   //png_set_swap(png_ptr);

   /* Swap bits of 1, 2, 4 bit packed pixel formats */
   //png_set_packswap(png_ptr);

   /* Turn on interlace handling if you are not using png_write_image() */
   /*if (interlacing)
      number_passes = png_set_interlace_handling(png_ptr);

   else
      number_passes = 1;*/

   /* The easiest way to write the image (you may have a different memory
    * layout, however, so choose what fits your needs best).  You need to
    * use the first method if you aren't handling interlacing yourself.
    */
	int bytes_per_pixel = 4;
	
   png_byte image[height][width*bytes_per_pixel];
   png_bytep row_pointers[height];

   if (height > PNG_UINT_32_MAX/sizeof(png_bytep))
     png_error (png_ptr, "Image is too tall to process in memory");

   for (int k = 0; k < height; k++)
     row_pointers[k] = imageData + k*width*bytes_per_pixel;

   /* One of the following output methods is REQUIRED */

/* Write out the entire image data in one call */
   png_write_image(png_ptr, row_pointers);

   /* The other way to write the image - deal with interlacing */


   /* You can write optional chunks like tEXt, zTXt, and tIME at the end
    * as well.  Shouldn't be necessary in 1.2.0 and up as all the public
    * chunks are supported and you can use png_set_unknown_chunks() to
    * register unknown chunks into the info structure to be written out.
    */

   /* It is REQUIRED to call this to finish writing the rest of the file */
   png_write_end(png_ptr, info_ptr);

   /* If you png_malloced a palette, free it here (don't free info_ptr->palette,
    * as recommended in versions 1.0.5m and earlier of this example; if
    * libpng mallocs info_ptr->palette, libpng will free it).  If you
    * allocated it with malloc() instead of png_malloc(), use free() instead
    * of png_free().
    */
   //png_free(png_ptr, palette);
   //palette = NULL;

   /* Similarly, if you png_malloced any data that you passed in with
    * png_set_something(), such as a hist or trans array, free it here,
    * when you can be sure that libpng is through with it.
    */
   //png_free(png_ptr, trans);
   //trans = NULL;
   /* Whenever you use png_free() it is a good idea to set the pointer to
    * NULL in case your application inadvertently tries to png_free() it
    * again.  When png_free() sees a NULL it returns without action, thus
    * avoiding the double-free security problem.
    */

   /* Clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&png_ptr, &info_ptr);

   /* Close the file */
   fclose(fp);
}


void dumpImage(uint8_t* data, int width, int height) {
	
	std::string filename = getenv("HOME") + std::string("/Downloads/image_") + std::to_string(rand()) + ".png";
	write_png(filename.c_str(), data, width, height);
}


