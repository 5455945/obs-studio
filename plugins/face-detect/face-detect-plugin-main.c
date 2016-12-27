#include <obs-module.h>
#include "face-detect.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("face-detect", "en-US")

bool obs_module_load(void)
{
	return true;
}

void obs_module_unload(void)
{
}
