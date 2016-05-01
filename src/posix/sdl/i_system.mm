#include <CoreFoundation/CoreFoundation.h>
#include "SDL.h"

void Mac_I_FatalError(const char* errortext)
{
	// Close window or exit fullscreen and release mouse capture
	SDL_Quit();
	
	const CFStringRef errorString = CFStringCreateWithCStringNoCopy( kCFAllocatorDefault, 
		errortext, kCFStringEncodingASCII, kCFAllocatorNull );
	if ( nullptr != errorString )
	{
		CFOptionFlags dummy;
	
		CFUserNotificationDisplayAlert( 0, kCFUserNotificationStopAlertLevel, nullptr, nullptr, nullptr, 
			CFSTR( "Fatal Error" ), errorString, CFSTR( "Exit" ), nullptr, nullptr, &dummy );
		CFRelease( errorString );
	}
}
