#include "python_code_container.hh"
#include "Text.hh"
#include "exception.hh"
#include "floats.hh"
#include "global.hh"

using namespace std;

map<string, bool>   PythonInstVisitor::gFunctionSymbolTable;
map<string, string> PythonInstVisitor::gMathLibTable;

map<string, bool>   PythonInstVisitor::gInstanceVariableTable;

dsp_factory_base* PythonCodeContainer::produceFactory()
{
    return new text_dsp_factory_aux(
        fKlassName, "", "",
        ((dynamic_cast<std::stringstream*>(fOut)) ? dynamic_cast<std::stringstream*>(fOut)->str() : ""), "");
}

CodeContainer* PythonCodeContainer::createScalarContainer(const string& name, int sub_container_type)
{
    return new PythonScalarCodeContainer(name, "", 0, 1, fOut, sub_container_type);
}

CodeContainer* PythonCodeContainer::createContainer(const string& name, const string& super, int numInputs,
                                                  int numOutputs, ostream* dst)
{
    if (gGlobal->gMemoryManager) {
        throw faustexception("ERROR : -mem not suported for Python\n");
    }
    if (gGlobal->gFloatSize == 3) {
        throw faustexception("ERROR : quad format not supported for Python\n");
    }
    if (gGlobal->gOpenCLSwitch) {
        throw faustexception("ERROR : OpenCL not supported for Python\n");
    }
    if (gGlobal->gCUDASwitch) {
        throw faustexception("ERROR : CUDA not supported for Python\n");
    }
    if (gGlobal->gOpenMPSwitch) {
        throw faustexception("ERROR : OpenMP not supported for Python\n");
    } else if (gGlobal->gSchedulerSwitch) {
        throw faustexception("ERROR : Scheduler not supported for Python\n");
    } else if (gGlobal->gVectorSwitch) {
        throw faustexception("ERROR : Vector mode not supported for Python\n");
    }

    return new PythonScalarCodeContainer(name, super, numInputs, numOutputs, dst, kInt);
}

// Scalar
PythonScalarCodeContainer::PythonScalarCodeContainer(const string& name, const string& super, int numInputs, int numOutputs,
                                                 std::ostream* out, int sub_container_type)
    : PythonCodeContainer(name, super, numInputs, numOutputs, out)
{
    fSubContainerType = sub_container_type;
}

PythonScalarCodeContainer::~PythonScalarCodeContainer()
{
}

void PythonCodeContainer::produceInternal()
{
    int n = 1;

    // Global declarations
    tab(n, *fOut);
    fCodeProducer.Tab(n);
    generateGlobalDeclarations(&fCodeProducer);

    tab(n, *fOut);
    *fOut << "final class " << fKlassName << " {";

    tab(n + 1, *fOut);
    tab(n + 1, *fOut);

    // Fields
    fCodeProducer.Tab(n + 1);
    generateDeclarations(&fCodeProducer);

    tab(n + 1, *fOut);
    // fKlassName used in method naming for subclasses
    // TODO: KlassName -> ClassName?
    produceInfoFunctions(n + 1, fKlassName, "dsp", true, false, &fCodeProducer);

    // TODO-j
    // generateInstanceInitFun("instanceInit" + fKlassName, true, false)->accept(&fCodeProducer);

    // Inits
    tab(n + 1, *fOut);
    *fOut << "def instanceInit" << fKlassName << "(samplingFreq):";
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateInit(&fCodeProducer);
    generateResetUserInterface(&fCodeProducer);
    generateClear(&fCodeProducer);
    tab(n + 1, *fOut);
    *fOut << "}";

    // Fill
    string counter = "count";
    if (fSubContainerType == kInt) {
        tab(n + 1, *fOut);
        *fOut << "def fill" << fKlassName << subst("($0, output):", counter);
    } else {
        tab(n + 1, *fOut);
        //*fOut << "void fill" << fKlassName << subst("($0, $1[] output) {", counter, ifloat());
        *fOut << "def fill" << fKlassName << subst("($0, output):", counter, ifloat());
    }
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateComputeBlock(&fCodeProducer);
    ForLoopInst* loop = fCurLoop->generateScalarLoop(counter);
    loop->accept(&fCodeProducer);
    // tab(n + 1, *fOut);
    // *fOut << "}";

    //tab(n, *fOut);
    //*fOut << "};" << endl;

    // Memory methods (as globals)
    tab(n, *fOut);
    *fOut << "def new" << fKlassName << "():";
    tab(n+1, *fOut);
    *fOut << "return new " << fKlassName << "()";

    tab(n, *fOut);
    *fOut << "def delete" << fKlassName << "(inst):";
    tab(n+1, *fOut);
    *fOut << "pass";
    tab(n, *fOut);
}

void PythonCodeContainer::produceClass()
{
    int n = 0;

    // tab(n+1, *fOut);
    // Libraries
    printLibrary(*fOut);


    // Early global stuff
    tab(n, *fOut);
    *fOut << "class FaustVarAccess:";
    tab(n+1, *fOut);
    *fOut << "def __init__(self, target, varname):";
    tab(n+2, *fOut);
    *fOut << "self.target = target";
    tab(n+2, *fOut);
    *fOut << "self.varname = varname";
    tab(n+1, *fOut);
    *fOut << "def get(self):";
    tab(n+2, *fOut);
    *fOut << "return self.target.__getattribute__(self.varname)";
    tab(n+1, *fOut);
    *fOut << "def set(self, val):";
    tab(n+2, *fOut);
    *fOut << "self.target.__setattr__(self.varname, val)";
    tab(n+1, *fOut);
    *fOut << "def getId(self):";
    tab(n+2, *fOut);
    *fOut << "return self.varname";

    tab(n, *fOut);
    //*fOut << "public class " << fKlassName << " extends " << fSuperKlassName << " {";
    *fOut << "class " << fKlassName << ":";

    // Global declarations
    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    fCodeProducer.Tab(n + 1);
    generateGlobalDeclarations(&fCodeProducer);

    // Sub containers
    generateSubContainers();

    // Fields
    tab(n + 1, *fOut);
    fCodeProducer.Tab(n + 1);
    generateDeclarations(&fCodeProducer);

    if (fAllocateInstructions->fCode.size() > 0) {
        tab(n + 1, *fOut);
        *fOut << "def allocate():";
        tab(n + 2, *fOut);
        fCodeProducer.Tab(n + 2);
        generateAllocate(&fCodeProducer);
        tab(n + 1, *fOut);
        *fOut << "}";
        tab(n + 1, *fOut);
    }

    if (fDestroyInstructions->fCode.size() > 0) {
        tab(n + 1, *fOut);
        *fOut << "def destroy():";
        tab(n + 2, *fOut);
        fCodeProducer.Tab(n + 2);
        generateDestroy(&fCodeProducer);
        tab(n + 1, *fOut);
        // *fOut << "}";
        tab(n + 1, *fOut);
    }

    // Print metadata declaration
    tab(n + 1, *fOut);

    *fOut << "def metadata(self, m): ";

    for (MetaDataSet::iterator i = gGlobal->gMetaDataSet.begin(); i != gGlobal->gMetaDataSet.end(); i++) {
        if (i->first != tree("author")) {
            tab(n + 2, *fOut);
            *fOut << "m.declare(\"" << *(i->first) << "\", " << **(i->second.begin()) << ")";
        } else {
            for (set<Tree>::iterator j = i->second.begin(); j != i->second.end(); j++) {
                if (j == i->second.begin()) {
                    tab(n + 2, *fOut);
                    *fOut << "m.declare(\"" << *(i->first) << "\", " << **j << ")";
                } else {
                    tab(n + 2, *fOut);
                    *fOut << "m.declare(\""
                          << "contributor"
                          << "\", " << **j << ")";
                }
            }
        }
    }

    // tab(n + 1, *fOut);
    // *fOut endl;

    tab(n + 1, *fOut);
    // No class name for main class
    produceInfoFunctions(n + 1, "", "dsp", true, true, &fCodeProducer);

    // Inits

    // TODO-j
    /*
    generateStaticInitFun("classInit", false)->accept(&fCodeProducer);
    generateInstanceInitFun("instanceInit", true, true)->accept(&fCodeProducer);
    */

    tab(n + 1, *fOut);
    *fOut << "def classInit(self, samplingFreq):";
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateStaticInit(&fCodeProducer);
    // tab(n + 1, *fOut);
    // *fOut << "}";

    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    *fOut << "def instanceConstants(self, samplingFreq):";
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateInit(&fCodeProducer);
    // tab(n + 1, *fOut);
    // *fOut << "}";
    

    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    *fOut << "def instanceResetUserInterface(self): ";
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateResetUserInterface(&fCodeProducer);
    // tab(n + 1, *fOut);
    // *fOut << "}";

    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    *fOut << "def instanceClear(self): ";
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateClear(&fCodeProducer);
    // tab(n + 1, *fOut);
    // *fOut << "}";

    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    *fOut << "def init(self, samplingFreq):";
    tab(n + 2, *fOut);
    *fOut << "classInit(samplingFreq);";
    tab(n + 2, *fOut);
    *fOut << "instanceInit(samplingFreq);";
    //tab(n + 1, *fOut);
    //*fOut << "}";

    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    *fOut << "def instanceInit(samplingFreq):";
    tab(n + 2, *fOut);
    *fOut << "instanceConstants(samplingFreq);";
    tab(n + 2, *fOut);
    *fOut << "instanceResetUserInterface();";
    tab(n + 2, *fOut);
    *fOut << "instanceClear();";
    //tab(n + 1, *fOut);
    //*fOut << "}";

    // User interface
    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    *fOut << "def buildUserInterface(self, ui_interface):";
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateUserInterface(&fCodeProducer);
    printlines(n + 2, fUICode, *fOut);
    //tab(n + 1, *fOut);
    //*fOut << "}";

    // Compute
    generateCompute(n);

    // Possibly generate separated functions
    fCodeProducer.Tab(n + 1);
    tab(n + 1, *fOut);
    generateComputeFunctions(&fCodeProducer);

    //tab(n, *fOut);
    //*fOut << "};\n" << endl;
    tab(n, *fOut);
}

void PythonScalarCodeContainer::generateCompute(int n)
{
    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    *fOut << subst("def compute(self, $0, $1[][] inputs, $1[][] outputs):", fFullCount, ifloat());
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);

    // Generates local variables declaration and setup
    generateComputeBlock(&fCodeProducer);

    // Generates one single scalar loop
    ForLoopInst* loop = fCurLoop->generateScalarLoop(fFullCount);
    loop->accept(&fCodeProducer);

    //tab(n + 1, *fOut);
    //*fOut << "}";
}
