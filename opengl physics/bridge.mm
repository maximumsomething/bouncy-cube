#include "bridge.hpp"

#import <Foundation/Foundation.h>





std::string getResourcesPath() {
	
	return [[[NSBundle mainBundle] resourcePath] UTF8String];
}
