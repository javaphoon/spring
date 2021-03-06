#ifndef MOUSE_INPUT_H
#define MOUSE_INPUT_H

#include <SDL_events.h>
#include <boost/signals/connection.hpp>

#include "Vec2.h"

class int2;

class IMouseInput
{
public:
	static IMouseInput* Get();

	IMouseInput ();
	virtual ~IMouseInput();

	virtual int2 GetPos ()
	{
		return mousepos;
	};
	void SetPos (int2 pos);

	virtual void Update () {}

	bool HandleSDLMouseEvent (const SDL_Event& event);

	virtual void SetWMMouseCursor (void* wmcursor) {}
	
protected:
	int2 mousepos;
	boost::signals::connection inputCon;
};


extern IMouseInput *mouseInput;

#endif 

