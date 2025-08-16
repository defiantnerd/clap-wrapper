// 

#include "plugview.h"
#include "wrapper.h"

#include "AAX_IViewContainer.h"
#include "AAX_CAutoreleasePool.h"

class Wrapped_AAX_GUI : public AAX_CEffectGUI
{
 public:
  Wrapped_AAX_GUI();
  ~Wrapped_AAX_GUI() AAX_OVERRIDE;

 protected:
  // AAX_CEffectGUI
  void CreateViewContents() AAX_OVERRIDE;
  void CreateViewContainer() AAX_OVERRIDE;
  void DeleteViewContainer() AAX_OVERRIDE;

  AAX_Result GetViewSize(AAX_Point* oEffectViewSize) const AAX_OVERRIDE;

  // Wrapped_AAX_GUI
  virtual void CreateEffectView(void* inSystemWindow);

 protected:
  ClapAsAAX* _clap = nullptr;
  const clap_plugin_t* _plugin = nullptr;
  const clap_plugin_gui_t* _gui = nullptr;
	clap_window_t _platformwindow;
  bool _created = false;
};

AAX_IEffectGUI* AAX_CALLBACK Wrapped_AAX_GUI_Create(void)
{
	return new Wrapped_AAX_GUI;
}

Wrapped_AAX_GUI::Wrapped_AAX_GUI()
{
	// init if necessary
	// mViewComponent = NULL

}
Wrapped_AAX_GUI::~Wrapped_AAX_GUI()
{
  AAX_CAutoreleasePool autorelease;
  // shutdown UI
  if (_gui && _created)
  {
    _gui->destroy(_plugin);
  }
  _clap = nullptr;
	_gui = nullptr;
	_plugin = nullptr;

  //if (mViewComponent)
  //{
  //	mViewComponent->forget();
  //	mViewComponent = NULL;
  //}
}

void Wrapped_AAX_GUI::CreateViewContents()
{
	// loading views and resources
}

void Wrapped_AAX_GUI::CreateEffectView (void *inSystemWindow)
{
	// mViewComponent = new VSTGUI_ContentView(inSystemWindow, this->GetEffectParameters(), this);
	// how do we get our plugin. the parameters have been set before
	// and GetEffectParameters() retrieves a pointer to AAX_IEffectParameters
	// 
	// 
#if WIN32
	#define CLAP_WINDOW_API CLAP_WINDOW_API_WIN32;
#elif MAC
	#define CLAP_WINDOW_API CLAP_WINDOW_API_COCOA;
#endif
	_platformwindow.api = CLAP_WINDOW_API;
	_platformwindow.ptr = inSystemWindow;
#undef CLAP_WINDOW_API

	auto params = this->GetEffectParameters();
	_clap = dynamic_cast<ClapAsAAX*>(params);
	if (_clap)
	{
		_gui = _clap->_plugin->_ext._gui;
		_plugin = _clap->_plugin->_plugin;

		if (_gui->create(_plugin, CLAP_WINDOW_API_WIN32, false))
		{
			_created = true;
			_gui->set_parent(_plugin, &_platformwindow);			
			_gui->set_scale(_plugin, 1.0);
			//clap_gui_resize_hints_t t;
			//_gui->get_resize_hints(_plugin, &t);
		}
	}
	// if (mViewComponent)
	{
		// mViewComponent->setBackgroundColor(VSTGUI::kGreyCColor);
	}
}

void Wrapped_AAX_GUI::CreateViewContainer ()
{
	if ( this->GetViewContainerType () == AAX_eViewContainer_Type_HWND )
	{
		this->CreateEffectView ( this->GetViewContainerPtr () );
	}
}

void Wrapped_AAX_GUI::DeleteViewContainer()
{
		AAX_CAutoreleasePool autorelease;

		if (_gui && _created)
		{
			_gui->destroy(_plugin);
			_created = false;
		}
	//if (mViewComponent)
	//{
	//	mViewComponent->forget();
                //	mViewComponent = NULL;
                //}
}

AAX_Result Wrapped_AAX_GUI::GetViewSize(AAX_Point* oEffectViewSize) const
{
	uint32_t w, h;
	if (_gui->get_size(_plugin, &w, &h))
	{
		oEffectViewSize->horz = w;
		oEffectViewSize->vert = h;
	}
	else
	{
		oEffectViewSize->horz = 800;
		oEffectViewSize->vert = 400;
	}

	return AAX_SUCCESS;	
}
