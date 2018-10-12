
process = environment {
	import("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/effects/layout2.dsp");
	import("https://raw.githubusercontent.com/grame-cncm/faust/simpler-SAM/examples/SAM/virtualAnalog/virtualAnalog-code.dsp");
}.process;

effect = _,_ : +
	: component("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/echo/echoForBrowser.dsp")
	: component("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/flanger/flangerForBrowser.dsp")
	: component("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/chorus/chorusForBrowser.dsp")
	: component("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/freeverb/freeverbForBrowser.dsp");
