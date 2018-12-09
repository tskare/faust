#ifndef _PYTHON_INSTRUCTIONS_H
#define _PYTHON_INSTRUCTIONS_H

using namespace std;

#include "text_instructions.hh"
#include "typing_instructions.hh"

class PythonInstVisitor : public TextInstVisitor {
   private:
    // Global table of function names; static in the visitor so that each
    // function prototype is generated as most once in the module.
    static map<string, bool>   gFunctionSymbolTable;
    static map<string, string> gMathLibTable;
    // Set of variables declared as properties.
    // In Python, instance variables must be referred to by self.foo
    // On the other hand, some functions may declare local variables,
    // such as loop index variables.
    // TODO: Investigate if it's possible to shadow an instance
    // variable with another. What happens if we declare a property |i|?
    static map<string, bool>  gInstanceVariableTable;

    TypingVisitor fTypingVisitor;

   protected:
    void EndLine() override
    {
        // In contrast to the existing languages; Python does not use
        // end-of-line semicolons.
        if (fFinishLine) {
            // *fOut << ";";  // No semicolons round these parts!
            tab(fTab, *fOut);  // TODO: see if we can have the function no-op.
        }
    }

   public:
    using TextInstVisitor::visit;

    PythonInstVisitor(std::ostream* out, int tab = 0) : TextInstVisitor(out, ".", ifloat(), "[]", tab)
    {
        initMathTable();

        // TODO: make sure this holds.
        // Pointer to Python object is actually the object itself...
        fTypeManager->fTypeDirectTable[Typed::kObj_ptr] = "";
    }

    void initMathTable()
    {
        if (gMathLibTable.size()) {
            return;
        }

        // Integer math
        gMathLibTable["abs"]   = "abs";
        gMathLibTable["max_i"] = "max";
        gMathLibTable["min_i"] = "min";

        // Single-precision math
        gMathLibTable["fabsf"]  = "abs";
        gMathLibTable["acosf"]  = "math.acos";
        gMathLibTable["asinf"]  = "math.asin";
        gMathLibTable["atanf"]  = "math.atan";
        gMathLibTable["atan2f"] = "math.atan2";
        gMathLibTable["ceilf"]  = "math.ceil";
        gMathLibTable["cosf"]   = "math.cos";
        gMathLibTable["coshf"]  = "math.cosh";
        gMathLibTable["expf"]   = "math.exp";
        gMathLibTable["floorf"] = "math.floor";
        // TODO: CheckMe
        gMathLibTable["fmodf"]  = "math.fmod";
        gMathLibTable["logf"]   = "math.log";
        gMathLibTable["log10f"] = "math.log10";
        gMathLibTable["max_f"]  = "max";
        gMathLibTable["min_f"]  = "min";
        gMathLibTable["powf"]   = "math.pow";
        gMathLibTable["roundf"] = "round";
        gMathLibTable["sinf"]   = "math.sin";
        gMathLibTable["sinhf"]  = "math.sinh";
        gMathLibTable["sqrtf"]  = "math.sqrt";
        gMathLibTable["tanf"]   = "math.tan";
        gMathLibTable["tanhf"]  = "math.tanh";

        // Double-precision math
        gMathLibTable["fabs"]  = "math.abs";
        gMathLibTable["acos"]  = "math.acos";
        gMathLibTable["asin"]  = "math.asin";
        gMathLibTable["atan"]  = "math.atan";
        gMathLibTable["atan2"] = "math.atan2";
        gMathLibTable["ceil"]  = "math.ceil";
        gMathLibTable["cos"]   = "math.cos";
        gMathLibTable["cosh"]  = "math.cosh";
        gMathLibTable["exp"]   = "math.exp";
        gMathLibTable["floor"] = "math.floor";
        // TODO: CheckMe
        gMathLibTable["fmod"]  = "math.fmod";
        gMathLibTable["log"]   = "math.log";
        gMathLibTable["log10"] = "math.log10";
        gMathLibTable["max_"]  = "max";
        gMathLibTable["min_"]  = "min";
        gMathLibTable["pow"]   = "math.pow";
        gMathLibTable["round"] = "round";
        gMathLibTable["sin"]   = "math.sin";
        gMathLibTable["sinh"]  = "math.sinh";
        gMathLibTable["sqrt"]  = "math.sqrt";
        gMathLibTable["tan"]   = "math.tan";
        gMathLibTable["tanh"]  = "math.tanh";
    }

    virtual ~PythonInstVisitor() {}

    string createVarAccess(string varname)
    {
        // TODO: we'll need to define the logic somewhere.
        return "(new FaustVarAccess(self, \"" + varname + "\"))";

        //j
        /*
        if (strcmp(ifloat(), "float") == 0) {
            return "(new FaustVarAccess(self, \"" + varname + "\"))"
            return "new FaustVarAccess() {\n"
                   "\t\t\t\tpublic String getId() { return \"" +
                   varname +
                   "\"; }\n"
                   "\t\t\t\tpublic void set(float val) { " +
                   varname +
                   " = val; }\n"
                   "\t\t\t\tpublic float get() { return (float)" +
                   varname +
                   "; }\n"
                   "\t\t\t}\n"
                   "\t\t\t";
        } else {
            return "new FaustVarAccess() {\n"
                   "\t\t\t\tpublic String getId() { return \"" +
                   varname +
                   "\"; }\n"
                   "\t\t\t\tpublic void set(double val) { " +
                   varname +
                   " = val; }\n"
                   "\t\t\t\tpublic float get() { return (double)" +
                   varname +
                   "; }\n"
                   "\t\t\t}\n"
                   "\t\t\t";
        }
        */
    }

    virtual void visit(AddMetaDeclareInst* inst)
    {
        *fOut << "ui_interface.declare(\"" << inst->fZone << "\", \"" << inst->fKey << "\", \"" << inst->fValue
              << "\")";
        EndLine();
    }

    virtual void visit(OpenboxInst* inst)
    {
        string name;
        switch (inst->fOrient) {
            case 0:
                name = "ui_interface.openVerticalBox(";
                break;
            case 1:
                name = "ui_interface.openHorizontalBox(";
                break;
            case 2:
                name = "ui_interface.openTabBox(";
                break;
        }
        *fOut << name << quote(inst->fName) << ")";
        EndLine();
    }

    virtual void visit(CloseboxInst* inst)
    {
        *fOut << "ui_interface.closeBox()";
        // TODO: endline?
        tab(fTab, *fOut);
    }

    virtual void visit(AddButtonInst* inst)
    {
        string name;
        if (inst->fType == AddButtonInst::kDefaultButton) {
            name = "ui_interface.addButton(";
        } else {
            name = "ui_interface.addCheckButton(";
        }
        *fOut << name << quote(inst->fLabel) << ", " << createVarAccess(inst->fZone) << ")";
        EndLine();
    }

    virtual void visit(AddSliderInst* inst)
    {
        string name;
        switch (inst->fType) {
            case AddSliderInst::kHorizontal:
                name = "ui_interface.addHorizontalSlider(";
                break;
            case AddSliderInst::kVertical:
                name = "ui_interface.addVerticalSlider(";
                break;
            case AddSliderInst::kNumEntry:
                name = "ui_interface.addNumEntry(";
                break;
        }
        *fOut << name << quote(inst->fLabel) << ", " << createVarAccess(inst->fZone) << ", " << checkReal(inst->fInit)
              << ", " << checkReal(inst->fMin) << ", " << checkReal(inst->fMax) << ", " << checkReal(inst->fStep)
              << ")";
        EndLine();
    }

    virtual void visit(AddBargraphInst* inst)
    {
        string name;
        switch (inst->fType) {
            case AddBargraphInst::kHorizontal:
                name = "ui_interface.addHorizontalBargraph(";
                break;
            case AddBargraphInst::kVertical:
                name = "ui_interface.addVerticalBargraph(";
                break;
        }
        *fOut << name << quote(inst->fLabel) << ", " << createVarAccess(inst->fZone) << ", " << checkReal(inst->fMin)
              << ", " << checkReal(inst->fMax) << ")";
        EndLine();
    }

    virtual void visit(LabelInst* inst) {}

    virtual void visit(DeclareVarInst* inst)
    {
        // TODO: could we replace the instance variable map with this check?
        bool tentative_is_instance = 
            (inst->fAddress->getAccess() & Address::kStruct);
        if (tentative_is_instance) {
            gInstanceVariableTable[inst->fAddress->getName()] = true;
        }
        string prefix = tentative_is_instance ? "self." : "";
        if (inst->fValue) {
            *fOut << prefix << inst->fAddress->getName() << " = ";
            inst->fValue->accept(this);
        } else {
            ArrayTyped* array_typed = dynamic_cast<ArrayTyped*>(inst->fType);
            // TODO: Consider using fTypeManager
            // though at runtime this shouldn't matter.
            if (array_typed && array_typed->fSize > 1) {
                string baseElem = (array_typed->fType->getType() == Typed::kFloat) ? "0.0" : "0";
                *fOut << prefix << inst->fAddress->getName() << " = (" <<
                    array_typed->fSize << "*[" << baseElem << "])";
            } else {
                // CHECK: we need to initialize this to something, None or 0 should work?
                // How do we get here in practice?
                *fOut << prefix << inst->fAddress->getName() << "=None";
            }
        }
        EndLine();

        //j
        /*
        if (inst->fAddress->getAccess() & Address::kStaticStruct) {
            *fOut << "static ";
        }

        ArrayTyped* array_typed = dynamic_cast<ArrayTyped*>(inst->fType);
        if (array_typed && array_typed->fSize > 1) {
            string type = fTypeManager->fTypeDirectTable[array_typed->fType->getType()];
            if (inst->fValue) {
                *fOut << type << " " << inst->fAddress->getName() << "[] = ";
                inst->fValue->accept(this);
            } else {
                *fOut << type << " " << inst->fAddress->getName() << "[] = new " << type << "[" << array_typed->fSize
                      << "]";
            }
        } else {
            *fOut << fTypeManager->generateType(inst->fType, inst->fAddress->getName());
            if (inst->fValue) {
                *fOut << " = ";
                inst->fValue->accept(this);
            }
        }
        */

        EndLine();
    }

    virtual void visit(DeclareFunInst* inst)
    {
        auto& name = inst->fName;
        // Already generated
        if (gFunctionSymbolTable.find(name) != gFunctionSymbolTable.end()) {
            return;
        } else {
            gFunctionSymbolTable[name] = true;
        }

        // Do not declare Math library functions; they call Python library code.
        if (gMathLibTable.find(name) != gMathLibTable.end()) {
            return;
        }

        // Prototype
        *fOut << fTypeManager->generateType(inst->fType->fResult, generateFunName(inst->fName));
        generateFunDefArgs(inst);
        generateFunDefBody(inst);
    }

    virtual void visit(LoadVarInst* inst)
    {
        fTypingVisitor.visit(inst);

        const string& name = inst->getName();
        bool is_instance_var =
            (gInstanceVariableTable.find(name) != gInstanceVariableTable.end());
        if (is_instance_var) {
            *fOut << "self.";
        }
        inst->fAddress->accept(this);
    }

    virtual void visit(StoreVarInst* inst)
    {
        const string& name = inst->getName();
        bool is_instance_var =
            (gInstanceVariableTable.find(name) != gInstanceVariableTable.end());
        if (is_instance_var) {
            *fOut << "self.";
        }
        inst->fAddress->accept(this);
        *fOut << " = ";
        inst->fValue->accept(this);
        EndLine();
    }

    virtual void visit(LoadVarAddressInst* inst)
    {
        // TODO: Need to implement this? Could build an index if needed.
        faustassert(!"visit(LoadVarAddressInst) not implemented in Python.");
    }

    virtual void visit(FloatNumInst* inst)
    {
        // TODO: need to remove float suffix, but it's likely instead better
        // to override in language settings at the global level.
        fTypingVisitor.visit(inst);
        TextInstVisitor::visit(inst);
    }

    virtual void visit(Int32NumInst* inst)
    {
        fTypingVisitor.visit(inst);
        TextInstVisitor::visit(inst);
    }

    virtual void visit(BoolNumInst* inst)
    {
        fTypingVisitor.visit(inst);
        TextInstVisitor::visit(inst);
    }

    virtual void visit(DoubleNumInst* inst)
    {
        fTypingVisitor.visit(inst);
        TextInstVisitor::visit(inst);
    }

    virtual void visit(BinopInst* inst)
    {
        if (isBoolOpcode(inst->fOpcode)) {
            *fOut << "(";
            inst->fInst1->accept(this);
            *fOut << " ";
            *fOut << gBinOpTable[inst->fOpcode]->fName;
            *fOut << " ";
            inst->fInst2->accept(this);
            *fOut << ")";
        } else {
            inst->fInst1->accept(&fTypingVisitor);
            Typed::VarType type1 = fTypingVisitor.fCurType;

            inst->fInst2->accept(&fTypingVisitor);
            Typed::VarType type2 = fTypingVisitor.fCurType;

            *fOut << "(";

            if (type1 == Typed::kInt32 && type2 == Typed::kInt32) {
                inst->fInst1->accept(this);
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                inst->fInst2->accept(this);
            } else if (type1 == Typed::kInt32 && type2 == Typed::kFloat) {
                // TODO: consolidate these blocks? Python _should_ auto-convert.
                //*fOut << "(float)";
                inst->fInst1->accept(this);
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                inst->fInst2->accept(this);
            } else if (type1 == Typed::kFloat && type2 == Typed::kInt32) {
                inst->fInst1->accept(this);
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                //*fOut << "(float)";
                inst->fInst2->accept(this);
            } else if (type1 == Typed::kFloat && type2 == Typed::kFloat) {
                inst->fInst1->accept(this);
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                inst->fInst2->accept(this);
            } else if (type1 == Typed::kInt32 && type2 == Typed::kBool) {
                inst->fInst1->accept(this);
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                *fOut << "(int(";
                inst->fInst2->accept(this);
                *fOut << "))";
            } else if (type1 == Typed::kBool && type2 == Typed::kInt32) {
                *fOut << "(int(";
                inst->fInst1->accept(this);
                *fOut << "))";
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                inst->fInst2->accept(this);
            } else if (type1 == Typed::kBool && type2 == Typed::kBool) {
                *fOut << "(int(";
                inst->fInst1->accept(this);
                *fOut << "))";
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                *fOut << "(int(";
                inst->fInst2->accept(this);
                *fOut << "))";
            } else if (type1 == Typed::kFloat && type2 == Typed::kBool) {
                inst->fInst1->accept(this);
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                *fOut << "(float(";
                inst->fInst2->accept(this);
                *fOut << ")?1.f:0.f)";
            } else if (type1 == Typed::kBool && type2 == Typed::kFloat) {
                *fOut << "(float(";
                inst->fInst1->accept(this);
                *fOut << "))";
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                inst->fInst2->accept(this);
            } else {  // Default
                inst->fInst1->accept(this);
                *fOut << " ";
                *fOut << gBinOpTable[inst->fOpcode]->fName;
                *fOut << " ";
                inst->fInst2->accept(this);
            }

            *fOut << ")";
        }

        fTypingVisitor.visit(inst);
    }

    virtual void visit(::CastInst* inst)
    {
        inst->fInst->accept(&fTypingVisitor);

        if (fTypeManager->generateType(inst->fType) == "int") {
            switch (fTypingVisitor.fCurType) {
                case Typed::kDouble:
                case Typed::kFloat:
                case Typed::kFloatMacro:
                    *fOut << "(int(";
                    inst->fInst->accept(this);
                    *fOut << "))";
                    break;
                case Typed::kInt32:
                    inst->fInst->accept(this);
                    break;
                case Typed::kBool:
                    *fOut << "(int(";
                    inst->fInst->accept(this);
                    *fOut << "))";
                    break;
                default:
                    printf("visitor.fCurType %d\n", fTypingVisitor.fCurType);
                    faustassert(false);
                    break;
            }
        } else {
            switch (fTypingVisitor.fCurType) {
                case Typed::kDouble:
                case Typed::kInt32:
                    *fOut << "(float(";
                    inst->fInst->accept(this);
                    *fOut << "))";
                    break;
                case Typed::kFloat:
                case Typed::kFloatMacro:
                    inst->fInst->accept(this);
                    break;
                case Typed::kBool:
                    *fOut << "(float(";
                    inst->fInst->accept(this);
                    *fOut << "))";
                    break;
                default:
                    printf("visitor.fCurType %d\n", fTypingVisitor.fCurType);
                    faustassert(false);
                    break;
            }
        }
        fTypingVisitor.visit(inst);
    }

    virtual void visit(BitcastInst* inst) { faustassert(false); }

    virtual void visit(FunCallInst* inst)
    {
        string fun_name =
            (gMathLibTable.find(inst->fName) != gMathLibTable.end()) ? gMathLibTable[inst->fName] : inst->fName;
        generateFunCall(inst, fun_name);
    }

    virtual void visit(Select2Inst* inst)
    {
        inst->fCond->accept(&fTypingVisitor);

        // ([true_expr] if [cond] else [false_expr])
        *fOut << "(";
        inst->fThen->accept(this);
        *fOut << " if ";

        switch (fTypingVisitor.fCurType) {
            case Typed::kDouble:
            case Typed::kInt32:
                *fOut << "(";
                inst->fCond->accept(this);
                *fOut << "==0)";
                break;
            case Typed::kFloat:
            case Typed::kFloatMacro:
                // TODO: should we treshold?
                *fOut << "(";
                inst->fCond->accept(this);
                *fOut << "==0.0)";
                break;
            case Typed::kBool:
                *fOut << "(";
                inst->fCond->accept(this);
                *fOut << ")";
                break;
            default:
                faustassert(false);
                break;
        }
        
        *fOut << " else ";
        inst->fElse->accept(this);
        *fOut << ")";

        fTypingVisitor.visit(inst);
    }

    static void cleanup()
    {
        gFunctionSymbolTable.clear();
        gMathLibTable.clear();
    }
};

#endif // _PYTHON_INSTRUCTIONS_H
