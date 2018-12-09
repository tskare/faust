#ifndef _PYTHON_CODE_CONTAINER_H
#define _PYTHON_CODE_CONTAINER_H

#include "code_container.hh"
#include "dsp_factory.hh"
#include "python_instructions.hh"

using namespace std;

class PythonCodeContainer : public virtual CodeContainer {
   protected:
    PythonInstVisitor fCodeProducer;
    std::ostream*     fOut;
    string            fSuperKlassName;

   public:
    PythonCodeContainer(const string& name, const string& super, int numInputs, int numOutputs, std::ostream* out)
        : fCodeProducer(out), fOut(out), fSuperKlassName(super)
    {
        initializeCodeContainer(numInputs, numOutputs);
        fKlassName = name;
    }
    virtual ~PythonCodeContainer() {}

    virtual void produceClass();
    virtual void generateCompute(int tab) = 0;
    void         produceInternal();

    virtual dsp_factory_base* produceFactory();

    virtual void printHeader() { CodeContainer::printHeader(*fOut); }

    CodeContainer* createScalarContainer(const string& name, int sub_container_type);

    static CodeContainer* createContainer(const string& name, const string& super, int numInputs, int numOutputs,
                                          ostream* dst = new stringstream());
};

class PythonScalarCodeContainer : public PythonCodeContainer {
   protected:
   public:
    PythonScalarCodeContainer(const string& name, const string& super, int numInputs, int numOutputs, std::ostream* out,
                              int sub_container_type);
    virtual ~PythonScalarCodeContainer();

    void generateCompute(int tab);
};

#endif // _PYTHON_CODE_CONTAINER_H
