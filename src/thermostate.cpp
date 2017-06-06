#include "node-refprop.h"

using namespace v8;
using namespace node;

// there should just be a way to return the thermostate object itself, but i haven't found it yet.
Local<Object> ThermoState::toJs() {
	Local<Object> obj = Object::New();
	obj->Set(String::NewSymbol("CP"), Number::New( this->CP ));
	obj->Set(String::NewSymbol("CV"), Number::New( this->CV ));
	obj->Set(String::NewSymbol("D"), Number::New( this->D ));
	obj->Set(String::NewSymbol("DL"), Number::New( this->DL ));
	obj->Set(String::NewSymbol("DV"), Number::New( this->DV ));
	obj->Set(String::NewSymbol("E"), Number::New( this->E ));
	obj->Set(String::NewSymbol("H"), Number::New( this->H ));
	obj->Set(String::NewSymbol("P"), Number::New( this->P ));
	obj->Set(String::NewSymbol("Q"), Number::New( this->Q ));
	obj->Set(String::NewSymbol("S"), Number::New( this->S ));
	obj->Set(String::NewSymbol("T"), Number::New( this->T ));
	obj->Set(String::NewSymbol("W"), Number::New( this->W ));
	obj->Set(String::NewSymbol("X"), Number::New( this->X ));
	obj->Set(String::NewSymbol("Y"), Number::New( this->Y ));
	obj->Set(String::NewSymbol("Z"), Number::New( this->Z ));
	
	this->trnprp->append(obj);
	
	return obj;
}

void OnePhaseTransport::append(v8::Local<v8::Object> obj) {
	obj->Set(String::NewSymbol("k"), Number::New( this->k ));
	obj->Set(String::NewSymbol("mu"), Number::New( this->mu ));
}

void TwoPhaseTransport::append(v8::Local<v8::Object> obj) {
	obj->Set(String::NewSymbol("kL"), Number::New( this->kL ));
	obj->Set(String::NewSymbol("muL"), Number::New( this->muL ));
	obj->Set(String::NewSymbol("kV"), Number::New( this->kV ));
	obj->Set(String::NewSymbol("muV"), Number::New( this->muV ));
	
	obj->Set(String::NewSymbol("CPL"), Number::New( this->CPL ));
	obj->Set(String::NewSymbol("CVL"), Number::New( this->CVL ));
	obj->Set(String::NewSymbol("CPV"), Number::New( this->CPV ));
	obj->Set(String::NewSymbol("CVV"), Number::New( this->CVV ));
	
	obj->Set(String::NewSymbol("sigma"), Number::New( this->sigma ));
}

// refprop's units:
/* temperature                     K
 * pressure, fugacity              kPa
 * density                         mol/L
 * composition                     mole fraction
 * quality                         mole basis (moles vapor/total moles)
 * enthalpy, internal energy       J/mol
 * Gibbs, Helmholtz free energy    J/mol
 * entropy, heat capacity          J/(mol.K)
 * speed of sound                  m/s
 * Joule-Thompson coefficient      K/kPa
 * d(p)/d(rho)                     kPa.L/mol
 * d2(p)/d(rho)2                   kPa.(L/mol)^2
 * viscosity                       microPa.s (10^-6 Pa.s)
 * thermal conductivity            W/(m.K)
 * dipole moment                   debye
 * surface tension                 N/m 
*/

ThermoState* RefpropContext::thermoState() {
	ThermoState *obj = new ThermoState();
	// assume pure fluid
	obj->X = 0; obj->Y = 0;
	obj->Z = 1;
	this->WMOLdll(&(obj->X), obj->molarMass);
	
	return obj;
}

// for these, molar mass is g/mol
void RefpropContext::toMolar(ThermoState *obj) {
	obj->E *= obj->molarMass * 1e-3;
	obj->H *= obj->molarMass * 1e-3;
	obj->S *= obj->molarMass * 1e-3;
	obj->D /= obj->molarMass;  // 1000 g/kg, 1000 L /m3 cancel
	obj->DL /= obj->molarMass;
	obj->DV /= obj->molarMass;
	obj->CP *= obj->molarMass * 1e-3;
	obj->CV *= obj->molarMass * 1e-3;
	
	// not really the same, but refprop deals in kPa, which is ridiculous.  convert here
	obj->P /= 1e3;
}

ThermoState* RefpropContext::doFlash(const char props[2], const double vals[2]) {	
	// look up the provided properties into the lookup table
	FlashFcn flashFcn = flashFcnLookup(props);
	
	if (NULL == flashFcn)
		ThrowException(Exception::TypeError(String::New("Property combination not supported!")));
	
	ThermoState *obj = this->thermoState();
	// now stuff the provided values into the thermostate structure
	for (int i=0; i < 2; i++) {
		switch(props[i]) { // TPDHSEQ
			case 'T':
				obj->T = vals[i];
				break;
			case 'P':
				obj->P = vals[i];
				break;
			case 'D':
				obj->D = vals[i];
				break;
			case 'H':
				obj->H = vals[i];
				break;
			case 'S':
				obj->S = vals[i];
				break;
			case 'E':
				obj->E = vals[i];
				break;
			case 'Q':
				obj->Q = vals[i];
				break;
		}
	}
	
	// convert qtys to molar, call the flash function, then bring the qtys back to specific
	this->toMolar(obj);
	(this->*flashFcn)(obj);
	this->doTransport(obj);
	this->toSpecific(obj);
	if (this->ierr != 0)
		ThrowException(Exception::TypeError(String::New(this->herr)));
	return obj;
}

void RefpropContext::toSpecific(ThermoState *obj) {
	obj->E /= obj->molarMass * 1e-3;
	obj->H /= obj->molarMass * 1e-3;
	obj->S /= obj->molarMass * 1e-3;
	obj->D *= obj->molarMass;  // 1000 g/kg, 1000 L /m3 cancel
	obj->DL *= obj->molarMass;
	obj->DV *= obj->molarMass;
	obj->CP /= obj->molarMass * 1e-3;
	obj->CV /= obj->molarMass * 1e-3;
	
	// not really the same, but refprop deals in kPa, which is ridiculous.  convert here
	obj->P *= 1e3;
}

void RefpropContext::calcTP(ThermoState* obj) {
	this->TPFLSHdll(obj->T,obj->P,&(obj->Z),obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->H,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcTD(ThermoState* obj) {
	this->TDFLSHdll(obj->T,obj->D,&(obj->Z),obj->P,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->H,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcTH(ThermoState* obj) {
	long kr;
	this->THFLSHdll(obj->T,obj->H,&(obj->Z),kr,obj->P,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcTS(ThermoState* obj) {
	long kr;
	this->TSFLSHdll(obj->T,obj->S,&(obj->Z),kr,obj->P,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->H,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcTE(ThermoState* obj) {
	long kr;
	this->TEFLSHdll(obj->T,obj->E,&(obj->Z),kr,obj->P,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->H,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcPD(ThermoState* obj) {
	this->PDFLSHdll(obj->P,obj->D,&(obj->Z),obj->T,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->H,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcPH(ThermoState* obj) {
	this->PHFLSHdll(obj->P,obj->H,&(obj->Z),obj->T,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}
void RefpropContext::calcPS(ThermoState* obj) {
	this->PSFLSHdll(obj->P,obj->S,&(obj->Z),obj->T,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->H,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcPE(ThermoState* obj) {
	this->PEFLSHdll(obj->P,obj->E,&(obj->Z),obj->T,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->H,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcHS(ThermoState* obj) {
	this->HSFLSHdll(obj->H,obj->S,&(obj->Z),obj->T,obj->P,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcES(ThermoState* obj) {
	this->ESFLSHdll(obj->E,obj->S,&(obj->Z),obj->T,obj->P,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->H,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcDH(ThermoState* obj) {
	this->DHFLSHdll(obj->D,obj->H,&(obj->Z),obj->T,obj->P,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcDS(ThermoState* obj) {
	this->DSFLSHdll(obj->D,obj->S,&(obj->Z),obj->T,obj->P,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->E,obj->H,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcDE(ThermoState* obj) {
	this->DEFLSHdll(obj->D,obj->E,&(obj->Z),obj->T,obj->P,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->Q,obj->H,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcTQ(ThermoState* obj) {
	long kq;
	this->TQFLSHdll(obj->T,obj->Q,&(obj->Z),kq,obj->P,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->E,obj->H,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::calcPQ(ThermoState* obj) {
	long kq;
	this->PQFLSHdll(obj->P,obj->Q,&(obj->Z),kq,obj->T,obj->D,obj->DL,obj->DV,&(obj->X),&(obj->Y),obj->E,obj->H,obj->S,obj->CV,obj->CP,obj->W,this->ierr,this->herr,errormessagelength);
}

void RefpropContext::doTransport(ThermoState* state) {
	// relevant refprop units:
	// viscosity                       microPa.s (10^-6 Pa.s)
	if (state->Q > 1 || state->Q < 0) {
		OnePhaseTransport *trns = new OnePhaseTransport();
		
		this->TRNPRPdll(state->T,state->D,&(state->Z),trns->mu,trns->k,this->ierr,this->herr,errormessagelength);
		trns->mu *= 1e-6;
		
		state->trnprp = trns;
	}
	else {
		TwoPhaseTransport *trns = new TwoPhaseTransport();
		
		this->TRNPRPdll(state->T,state->DL,&(state->Z),trns->muL,trns->kL,this->ierr,this->herr,errormessagelength);
		trns->muL *= 1e-6;
		this->TRNPRPdll(state->T,state->DV,&(state->Z),trns->muV,trns->kV,this->ierr,this->herr,errormessagelength);
		trns->muV *= 1e-6;
		this->SURTENdll(state->T,state->DL,state->DV,&(state->Z),&(state->Z),trns->sigma,this->ierr,this->herr,errormessagelength);
		
		this->CVCPdll(state->T,state->DL,&(state->Z),trns->CVL,trns->CPL);
		this->CVCPdll(state->T,state->DV,&(state->Z),trns->CVV,trns->CPV);
		
		// convert from molar-specific to mass-specifc heats
		trns->CPL /= state->molarMass * 1e-3;
		trns->CVL /= state->molarMass * 1e-3;
		trns->CPV /= state->molarMass * 1e-3;
		trns->CVV /= state->molarMass * 1e-3;
	
		state->trnprp = trns;
	}
}