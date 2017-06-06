var refprop = require('node-refprop');
var should = require('should');
var assert = require('assert');

describe('refprop', function() {	
	it('should load without error', function() {
		assert.equal(refprop.setFluid('R134A'), undefined);
	});
	
	it('should throw an error for invalid fluids', function() {
		(function() {
			refprop.setFluid('urine');
		}).should.throw();
	});
	
	it('should compute single-phase states for pure fluids', function() {
		refprop.setFluid('nitrogen');
		
		var result = refprop.statePoint({T: 273.15, P: 101.3e3});
		result.should.be.Object;
		result.should.have.properties(["CP","CV","D","DL","DV","E","H","P","Q","S","T","W","X","k","mu"]);
		
		result.T.should.be.eql(273.15);
		result.P.should.be.eql(101.3e3);
		result.H.should.be.approximately(283.23e3, .01e3);
		result.D.should.be.approximately(1.2501, .0001);
		result.S.should.be.approximately(6.7442e3, .1);
	});
	
	it('should compute two-phase states for pure fluids', function() {
		refprop.setFluid('isobutan');
		
		var result = refprop.statePoint({T: 220, Q: .5});
		result.should.be.Object;
		result.should.have.properties(["CP","CV","D","DL","DV","E","H","P","Q","S","T","W","X",'CPL','CPV','kL','kV','CVL','CVV','muL','muV','sigma']);
		
		result.T.should.be.eql(220);
		result.P.should.be.approximately(14.023e3, .001e3);
		result.H.should.be.approximately(284.90e3, .01e3);
		result.D.should.be.approximately(.89931, .00001);
		result.S.should.be.approximately(1.4422e3, .0001e3);
		result.CPL.should.be.approximately(2.0413e3, .0001e3);
		result.CPV.should.be.approximately(1.3296e3, .0001e3);
		result.kL.should.be.approximately(.12056, .00001);
		result.kV.should.be.approximately(9.6201e-3, .0001);
	});
	
	it.skip('should compute states for pre-defined mixtures', function() {
		refprop.setFluid('R410A.ppf');
		
		var result = refprop.statePoint({T: 273.15, P: 101.3e3});
		result.should.be.Object;
		result.should.have.properties(["CP","CV","D","DL","DV","E","H","P","Q","S","T","W","X","k","mu"]);
		
		result.T.should.be.eql(273.15);
		result.P.should.be.eql(101.3e3);
		result.H.should.be.approximately(439.57e3, .01e3);
		result.D.should.be.approximately(3.2981, .0001);
		result.S.should.be.approximately(2.0983e3, .1);
	});
	
	it.skip('should compute states for custom mixtures', function() {
	});
});