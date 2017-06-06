#include <node.h>

#include <windows.h>

#include "node-refprop.h"

using namespace v8;
using namespace node;

RefpropContext* RefpropContext::_instance = NULL;

// singleton pattern
RefpropContext* RefpropContext::instance() {
	if (!_instance)   // Only allow one instance of class to be generated.
		_instance = new RefpropContext;
	return _instance;
}

RefpropContext::RefpropContext() {
	this->RefpropDllInstance = LoadLibrary("C:\\Program Files (x86)\\REFPROP\\REFPRP64.DLL");
	
	if (NULL == this->RefpropDllInstance) {
		ThrowException(Exception::TypeError(String::New("Failed to load refprop.dll")));
		return;
	}
	
	this->SETUPdll = (fp_SETUPdllTYPE) GetProcAddress(this->RefpropDllInstance,"SETUPdll");
	if (NULL == this->SETUPdll) {
		ThrowException(Exception::TypeError(String::New("Failed to locate setup function pointer")));
		return;
	}
	
	// get function pointers into DLL
	this->DEFLSHdll = (fp_DEFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"DEFLSHdll");
	this->DHFLSHdll = (fp_DHFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"DHFLSHdll");
	this->DSFLSHdll = (fp_DSFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"DSFLSHdll");
	this->ESFLSHdll = (fp_ESFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"ESFLSHdll");
	this->HSFLSHdll = (fp_HSFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"HSFLSHdll");
	this->PDFLSHdll = (fp_PDFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"PDFLSHdll");
	this->PEFLSHdll = (fp_PEFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"PEFLSHdll");
	this->PHFLSHdll = (fp_PHFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"PHFLSHdll");
	this->PQFLSHdll = (fp_PQFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"PQFLSHdll");
	this->PSFLSHdll = (fp_PSFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"PSFLSHdll");
	this->TDFLSHdll = (fp_TDFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"TDFLSHdll");
	this->TEFLSHdll = (fp_TEFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"TEFLSHdll");
	this->THFLSHdll = (fp_THFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"THFLSHdll");
	this->TPFLSHdll = (fp_TPFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"TPFLSHdll");
	this->TQFLSHdll = (fp_TQFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"TQFLSHdll");
	this->TSFLSHdll = (fp_TSFLSHdllTYPE) GetProcAddress(this->RefpropDllInstance,"TSFLSHdll");
	this->WMOLdll = (fp_WMOLdllTYPE) GetProcAddress(this->RefpropDllInstance,"WMOLdll");
	this->TRNPRPdll = (fp_TRNPRPdllTYPE) GetProcAddress(this->RefpropDllInstance,"TRNPRPdll");
	this->CVCPdll = (fp_CVCPdllTYPE) GetProcAddress(this->RefpropDllInstance,"CVCPdll");
	this->SURTENdll = (fp_SURTENdllTYPE) GetProcAddress(this->RefpropDllInstance,"SURTENdll");
	
	// build the flash function lookup table
	this->flashString = "TPDHSEQ";
	
	this->flashTable[0][1] = &RefpropContext::calcTP;
	this->flashTable[0][2] = &RefpropContext::calcTD;
	this->flashTable[0][3] = &RefpropContext::calcTH;
	this->flashTable[0][4] = &RefpropContext::calcTS;
	this->flashTable[0][5] = &RefpropContext::calcTE;
	this->flashTable[0][6] = &RefpropContext::calcTQ;
	
	this->flashTable[1][2] = &RefpropContext::calcPD;
	this->flashTable[1][3] = &RefpropContext::calcPH;
	this->flashTable[1][4] = &RefpropContext::calcPS;
	this->flashTable[1][5] = &RefpropContext::calcPE;
	this->flashTable[1][6] = &RefpropContext::calcPQ;
	
	this->flashTable[2][3] = &RefpropContext::calcDH;
	this->flashTable[2][4] = &RefpropContext::calcDS;
	this->flashTable[2][5] = &RefpropContext::calcDE;
	
	this->flashTable[3][4] = &RefpropContext::calcHS;

	this->flashTable[4][5] = &RefpropContext::calcES; // would make more sense for this to be calcSE, but whatever
	
	// make it a symmetric matrix
	for (int i = 0; i < 7; i++)
		for (int j = 0; j < 7; j++)
			flashTable[j][i] = flashTable[i][j];
}

Handle<Value> setFluid(const Arguments& args) {
	HandleScope scope;
	
	RefpropContext* rp = RefpropContext::instance();

	// now we do error checking to ensure we can pop out the requestedFluid argument
    if (args.Length() < 1) {
        return ThrowException(Exception::TypeError(
            String::New("Must specify the fluid type when creating a refprop context")));
    }

    // now pull out the requestedFluid and cram it into a c-style string
	String::Utf8Value requestedFluid(args[0]->ToString());
	rp->setFluid(*requestedFluid);
	return scope.Close(Undefined());
}

Handle<Value> getFluid(const Arguments& args) {
	HandleScope scope;
	
	RefpropContext* rp = RefpropContext::instance();
	
	Local<String> fluid = String::New(rp->getFluid());
	return scope.Close(fluid);
}

void RefpropContext::setFluid(char *requestedFluid) {	
	// if they're just asking for the same string we've already loaded, don't bother doing anything
	if (strcmp(this->_fluid, requestedFluid) != 0) {
		long i,ierr=0;
		
		char hf[refpropcharlength*ncmax], hrf[lengthofreference],
			herr[errormessagelength],hfmix[refpropcharlength];

		// Explicitly set the fluid file PATH... this doesn't work generically but fuck it
		char *FLD_PATH = "C:\\Program Files (x86)\\REFPROP\\fluids\\";
		strcpy(hf,FLD_PATH);
		strcpy(hfmix,FLD_PATH);

		//...initialize the program and set the pure fluid component name
		i=1;
		strcat(hf,requestedFluid);
		strcat(hf,".FLD");
		strcat(hfmix,"HMX.BNC");
		strcpy(hrf,"DEF");
		strcpy(herr,"Ok");

		// when we get to this point, they've asked for a new fluid, so we'll load that mother into the existing refprop context
		this->SETUPdll(i, hf, hfmix, hrf, ierr, herr,refpropcharlength*ncmax,refpropcharlength,lengthofreference,errormessagelength);

		if (ierr != 0)
			ThrowException(Exception::TypeError(String::New("Error loading fluid requested fluid!")));
		else
			strcpy(this->_fluid, requestedFluid);
	}
}

char* RefpropContext::getFluid() {
	return this->_fluid;
}

Handle<Value> statePoint(const Arguments& args) {
	HandleScope scope;
	// there should be at least one and maybe two arguments
	// args[0] should be an object with two fields.  the keys should be used to lookup the correct flash function
	// args[1] might be an array with strings in it.  the strings represent requested properties
	//  - we'll go ahead and always reply with all the flash function properties, but give out whatever else they want
	
	if (args.Length() < 1) {
		return ThrowException(Exception::TypeError(String::New("Must provide quantities to establish thermodynamic state")));
	}
	
	// get the key/value pairs that establish the thermodynamic state
	Local<Object> coords = args[0]->ToObject()->Clone();  // note that the keys are available with coords->GetOwnPropertyNames();
	Local<Array> keys = coords->GetOwnPropertyNames();
	
	if (keys->Length() != 2) {
		return ThrowException(Exception::TypeError(String::New("Thermodynamic state established by exactly 2 values")));
	}
	
	char props[2];
	double values[2];
	
	for (int i=0; i < 2; i++) {
		String::Utf8Value key(keys->Get(i)->ToString());
		props[i] = (*key)[0];
		values[i] = coords->Get(keys->Get(i))->ToNumber()->Value();
	}
	
	// grab refprop
	RefpropContext* rp = RefpropContext::instance();
	ThermoState *state = rp->doFlash(props, values);
		
	return scope.Close(state->toJs());
}

RefpropContext::FlashFcn RefpropContext::flashFcnLookup(const char props[2]) {
	int idx[2];
	
	for (int i=0; i < 2; i++) {
		char* e = strchr(this->flashString, props[i]);
		if (e) {
			idx[i] = (int)(e - this->flashString);
		}
		else {
			ThrowException(Exception::TypeError(String::New("Provided thermodynamic quantities must be one of TPDHSEQ")));
			return NULL;
		}
	}
	return this->flashTable[idx[0]][idx[1]];
}

void RegisterModule(Handle<Object> exports) {
	// syntax for registering functions to the exports object is:
	// NODE_SET_METHOD(exports, "name_of_function", functionPointer);
	NODE_SET_METHOD(exports, "setFluid", setFluid);
	NODE_SET_METHOD(exports, "statePoint", statePoint);
	NODE_SET_METHOD(exports, "getFluid", getFluid);
}

NODE_MODULE(node_refprop, RegisterModule)