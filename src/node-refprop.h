#ifndef NODE_REFPROP_H
#define NODE_REFPROP_H

#include <node.h>
#include <windows.h>
#include "refprop1.h"

// constants for calling refprop... fortran calling conventions, basically
#define refpropcharlength 255
#define filepathlength 255
#define lengthofreference 3
#define errormessagelength 255
#define ncmax 20		// Note: ncmax is the max number of components
#define numparams 72 
#define maxcoefs 50

class TransportProps {
public:
	virtual void append(v8::Local<v8::Object> obj) = 0;
};

class OnePhaseTransport : public TransportProps {
public:
	double mu, k;
	void append(v8::Local<v8::Object> obj);
};

class TwoPhaseTransport : public TransportProps {
public:
	double muL, muV, kL, kV;
	double CPV, CPL, CVV, CVL; // do these belong here?  i don't know and i don't care
	double sigma;
	void append(v8::Local<v8::Object> obj);
};

class ThermoState {
public:
	v8::Local<v8::Object> toJs();
	
	double T;
	double P;
	double Z;
	double D;
	double DL;
	double DV;
	double X;
	double Y;
	double Q;
	double E;
	double H;
	double S;
	double CV;
	double CP;
	double W;
	double molarMass;
	
	TransportProps* trnprp;
};

v8::Handle<v8::Value> setFluid(const v8::Arguments& args);
v8::Handle<v8::Value> getFluid(const v8::Arguments& args);
v8::Handle<v8::Value> statePoint(const v8::Arguments& args);
	
class RefpropContext {
public:
	typedef void (RefpropContext::*FlashFcn)(ThermoState*);

	static RefpropContext* instance();  // singleton pattern
	
	void setFluid(char* reqdFluid);
	char* getFluid();
	ThermoState* doFlash(const char props[], const double vals[]);

private:
	RefpropContext();  // constructor private to enforce singleton pattern
	
	static RefpropContext* _instance;	
	char _fluid[refpropcharlength];
	long ierr;
	char herr[errormessagelength+1];

	HINSTANCE RefpropDllInstance;  // holds the DLL functions so we don't have to reload every time
	char* flashString;
	FlashFcn flashTable[7][7];
	FlashFcn flashFcnLookup(const char props[2]);
	
	ThermoState* thermoState();
	void toMolar(ThermoState *obj);
	void toSpecific(ThermoState *obj);

	void calcTP(ThermoState*);
	void calcTD(ThermoState*);
	void calcTH(ThermoState*);
	void calcTS(ThermoState*);
	void calcTE(ThermoState*);
	void calcPD(ThermoState*);
	void calcPH(ThermoState*);
	void calcPS(ThermoState*);
	void calcPE(ThermoState*);
	void calcHS(ThermoState*);
	void calcES(ThermoState*);
	void calcDH(ThermoState*);
	void calcDS(ThermoState*);
	void calcDE(ThermoState*);
	void calcTQ(ThermoState*);
	void calcPQ(ThermoState*);
	
	void doTransport(ThermoState* state);
	
	//Define explicit function pointers to refprop methods
	fp_ABFL1dllTYPE ABFL1dll;
	fp_ABFL2dllTYPE ABFL2dll;
	fp_ACTVYdllTYPE ACTVYdll;
	fp_AGdllTYPE AGdll;
	fp_CCRITdllTYPE CCRITdll;
	fp_CP0dllTYPE CP0dll;
	fp_CRITPdllTYPE CRITPdll;
	fp_CSATKdllTYPE CSATKdll;
	fp_CV2PKdllTYPE CV2PKdll;
	fp_CVCPKdllTYPE CVCPKdll;
	fp_CVCPdllTYPE CVCPdll;
	fp_DBDTdllTYPE DBDTdll;
	fp_DBFL1dllTYPE DBFL1dll;
	fp_DBFL2dllTYPE DBFL2dll;
	fp_DDDPdllTYPE DDDPdll;
	fp_DDDTdllTYPE DDDTdll;
	fp_DEFLSHdllTYPE DEFLSHdll;
	fp_DHD1dllTYPE DHD1dll;
	fp_DHFLSHdllTYPE DHFLSHdll;
	fp_DIELECdllTYPE DIELECdll;
	fp_DOTFILLdllTYPE DOTFILLdll;
	fp_DPDD2dllTYPE DPDD2dll;
	fp_DPDDKdllTYPE DPDDKdll;
	fp_DPDDdllTYPE DPDDdll;
	fp_DPDTKdllTYPE DPDTKdll;
	fp_DPDTdllTYPE DPDTdll;
	fp_DPTSATKdllTYPE DPTSATKdll;
	fp_DSFLSHdllTYPE DSFLSHdll;
	fp_ENTHALdllTYPE ENTHALdll;
	fp_ENTROdllTYPE ENTROdll;
	fp_ESFLSHdllTYPE ESFLSHdll;
	fp_FGCTYdllTYPE FGCTYdll;
	fp_FPVdllTYPE FPVdll;
	fp_GERG04dllTYPE GERG04dll;
	fp_GETFIJdllTYPE GETFIJdll;
	fp_GETKTVdllTYPE GETKTVdll;
	fp_GIBBSdllTYPE GIBBSdll;
	fp_HSFLSHdllTYPE HSFLSHdll;
	fp_INFOdllTYPE INFOdll;
	fp_LIMITKdllTYPE LIMITKdll;
	fp_LIMITSdllTYPE LIMITSdll;
	fp_LIMITXdllTYPE LIMITXdll;
	fp_MELTPdllTYPE MELTPdll;
	fp_MELTTdllTYPE MELTTdll;
	fp_MLTH2OdllTYPE MLTH2Odll;
	fp_NAMEdllTYPE NAMEdll;
	fp_PDFL1dllTYPE PDFL1dll;
	fp_PDFLSHdllTYPE PDFLSHdll;
	fp_PEFLSHdllTYPE PEFLSHdll;
	fp_PHFL1dllTYPE PHFL1dll;
	fp_PHFLSHdllTYPE PHFLSHdll;
	fp_PQFLSHdllTYPE PQFLSHdll;
	fp_PREOSdllTYPE PREOSdll;
	fp_PRESSdllTYPE PRESSdll;
	fp_PSFL1dllTYPE PSFL1dll;
	fp_PSFLSHdllTYPE PSFLSHdll;
	fp_PUREFLDdllTYPE PUREFLDdll;
	fp_QMASSdllTYPE QMASSdll;
	fp_QMOLEdllTYPE QMOLEdll;
	fp_SATDdllTYPE SATDdll;
	fp_SATEdllTYPE SATEdll;
	fp_SATHdllTYPE SATHdll;
	fp_SATPdllTYPE SATPdll;
	fp_SATSdllTYPE SATSdll;
	fp_SATTdllTYPE SATTdll;
	fp_SETAGAdllTYPE SETAGAdll;
	fp_SETKTVdllTYPE SETKTVdll;
	fp_SETMIXdllTYPE SETMIXdll;
	fp_SETMODdllTYPE SETMODdll;
	fp_SETREFdllTYPE SETREFdll;
	fp_SETUPdllTYPE SETUPdll;
	fp_SPECGRdllTYPE SPECGRdll;
	fp_SUBLPdllTYPE SUBLPdll;
	fp_SUBLTdllTYPE SUBLTdll;
	fp_SURFTdllTYPE SURFTdll;
	fp_SURTENdllTYPE SURTENdll;
	fp_TDFLSHdllTYPE TDFLSHdll;
	fp_TEFLSHdllTYPE TEFLSHdll;
	fp_THERM0dllTYPE THERM0dll;
	fp_THERM2dllTYPE THERM2dll;
	fp_THERM3dllTYPE THERM3dll;
	fp_THERMdllTYPE THERMdll;
	fp_THFLSHdllTYPE THFLSHdll;
	fp_TPFLSHdllTYPE TPFLSHdll;
	fp_TPRHOdllTYPE TPRHOdll;
	fp_TQFLSHdllTYPE TQFLSHdll;
	fp_TRNPRPdllTYPE TRNPRPdll;
	fp_TSFLSHdllTYPE TSFLSHdll;
	fp_VIRBdllTYPE VIRBdll;
	fp_VIRCdllTYPE VIRCdll;
	fp_WMOLdllTYPE WMOLdll;
	fp_XMASSdllTYPE XMASSdll;
	fp_XMOLEdllTYPE XMOLEdll;
};

#endif