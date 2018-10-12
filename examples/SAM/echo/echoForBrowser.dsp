
process = environment {
	import("https://raw.githubusercontent.com/grame-cncm/faust/simpler-SAM/examples/SAM/my-powForBrowser.dsp");
	import("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/effects/layout2.dsp");
	import("https://raw.githubusercontent.com/grame-cncm/faust/simpler-SAM/examples/SAM/echo/echo-code.dsp");
}.process;


