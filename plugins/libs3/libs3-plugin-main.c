#include <obs-module.h>
#include "libs3.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("libs3", "en-US")

bool obs_module_load(void)
{
	return true;
}

void obs_module_unload(void)
{
}
