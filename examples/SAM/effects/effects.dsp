// All effects used by minimoog.dsp

import("stdfaust.lib");
import("layout2.dsp");

process = _,_ : +
	: component("../echo/echo.dsp")
	: component("../flanger/flanger.dsp")
	: component("../chorus/chorus.dsp")
	: component("../freeverb/freeverb.dsp");
