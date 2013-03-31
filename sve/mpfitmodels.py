#
#    This file is part of Spectra Visual Editor (SVE).
#    Copyright 2011-2012 Sergi Blanco Cuaresma - http://www.marblestation.com
#
#    SVE is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    SVE is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with SVE. If not, see <http://www.gnu.org/licenses/>.
#
import mpfit
import numpy as np
import matplotlib.pyplot as plt
from scipy.integrate import quad

class MPFitModel(object):
    def __init__(self, p):
        # Parinfo: structure where the parameters properties are stored
        self._parinfo = []
        for i in np.arange(len(p)):
            self._parinfo.append({'value':p[i], 'fixed':False, 'limited':[False, False], 'limits':[0., 0.]})
        # Data for the fitting
        self.x = None
        self.y = None
        self.weights = None
        self.rms = None
        self.m = None # MPFIT object

    def __call__(self, x):
        return self._model_function(x)

    def _model_function(self, x, p=None):
        pass

    def _model_evaluation_function(self, p, fjac=None):
        # Function that return the weighted deviates

        # Parameter values are passed in "p"
        # If fjac==None then partial derivatives should not be
        # computed.  It will always be None if MPFIT is called with default
        # flag.
        model = self._model_function(self.x, p)
        # Non-negative status value means MPFIT should continue, negative means
        # stop the calculation.
        status = 0
        if self.weights is not None:
            return([status, (self.y - model)*self.weights])
        else:
            return([status, (self.y - model)])

    def fitData(self, x, y, weights=None, parinfo=None, ftol=1.e-10, xtol=1.e-10, gtol=1.e-10, damp=0, maxiter=200, quiet=True):
        """
        - ftol: Termination occurs when both the actual
                and predicted relative reductions in the sum of squares are at most
                ftol
        - xtol: Termination occurs when the relative error
                between two consecutive iterates is at most xtol
        - gtol: Termination occurs when the cosine of
                the angle between fvec and any column of the jacobian is at most gtol
                in absolute value
        - damp: Residuals bigger than "damp" are not considered (damped)
        - maxiter: Maximum number of iterations
        """
        self.x = x
        self.y = y
        self.weights = weights

        # Parameters' constraints
        if parinfo is not None:
            self._parinfo = parinfo

        m = mpfit.mpfit(self._model_evaluation_function, parinfo=self._parinfo, ftol=ftol, xtol=xtol, gtol=gtol, damp=damp, maxiter=maxiter, quiet=quiet)

        if (m.status <= 0):
           raise Exception(m.errmsg)
        else:
            for i in np.arange(len(m.params)):
                self._parinfo[i]['value'] = m.params[i]
            # Num iterations: m.niter
            # Uncertainties: m.perror
        self.m = m
        # Save RMS
        residuals = self.residuals()
        self.rms = np.sqrt(np.sum(np.power(residuals,2))/len(residuals))

    def residuals(self):
        model = self._model_function(self.x)
        return((self.y - model))

    def integrate(self):
        raise NotImplementedError()


class GaussianModel(MPFitModel):
    # WARNING: Dot not modify attributes A, sig or mu directly from outside the class!
    def __init__(self, baseline=0, A=-0.025, sig=0.25, mu=0):
        p = [baseline, A, sig, mu]
        super(GaussianModel, self).__init__(p)

    def _model_function(self, x, p=None):
        # The model function with parameters p required by mpfit library
        if p is not None:
            # Update internal structure for fitting:
            self._parinfo[0]['value'] = p[0]
            self._parinfo[1]['value'] = p[1]
            self._parinfo[2]['value'] = p[2]
            self._parinfo[3]['value'] = p[3]
        if self.sig() == 0:
            return self.baseline()
        else:
            return self.baseline() + ((self.A()*1.)/np.sqrt(2*np.pi*self.sig()**2))*np.exp(-(x-self.mu())**2/(2*self.sig()**2))

    def fitData(self, x, y, weights=None, parinfo=None):
        if len(parinfo) != 4:
            raise Exception("Wrong number of parameters!")
        super(GaussianModel, self).fitData(x, y, weights, parinfo)

    def baseline(self): return self._parinfo[0]['value']
    def A(self): return self._parinfo[1]['value']
    def sig(self): return self._parinfo[2]['value']
    def mu(self): return self._parinfo[3]['value']

    def _make_gauss(self):
        k = self.A() / (self.sig() * np.sqrt(2*np.pi))
        s = -1.0 / (2 * self.sig() * self.sig())
        def f(x):
            return k * np.exp(s * (x - self.mu())*(x - self.mu()))
        return f

    def integrate(self, from_x=None, to_x=None):
        # Define range: Include 99.9999998% of the gaussian area
        if from_x is None:
            from_x = self.mu() - 6*self.sig()
        if to_x is None:
            to_x = self.mu() + 6*self.sig()

        #if self.x is None:
            #return 0
        #else:
        integral, estimated_error = quad(self._make_gauss(), from_x, to_x)
        return integral

    # Returns fwhm in nm and kms
    def fwhm(self):
        # Light speed in vacuum
        c = 299792458.0 # m/s
        fwhm = self.sig() * (2*np.sqrt(2*np.log(2))) # nm
        fwhm_kms = (c / (self.mu() / fwhm)) / 1000.0 # km/s
        return fwhm, fwhm_kms

    def resolution(self):
        fwhm, fwhm_kms = self.fwhm()
        return self.mu() / fwhm



class VoigtModel(MPFitModel):
    # WARNING: Dot not modify attributes A, sig, mu or gamma directly from outside the class!
    def __init__(self, baseline=0, A=-0.025, sig=0.25, mu=0, gamma=0.025):
        p = [baseline, A, sig, mu, gamma]
        super(VoigtModel, self).__init__(p)

    def _model_function(self, x, p=None):
        # The model function with parameters p required by mpfit library
        if p is not None:
            # Update internal structure for fitting:
            self._parinfo[0]['value'] = p[0]
            self._parinfo[1]['value'] = p[1]
            self._parinfo[2]['value'] = p[2]
            self._parinfo[3]['value'] = p[3]
            self._parinfo[4]['value'] = p[4]
        if self.sig == 0:
            # Equivalent to a Lorentzian model
            voigt_result = self.baseline() + (self.A()*self.gamma()/np.pi/(x*x - 2*x*self.mu()+self.mu()*self.mu()+self.gamma()*self.gamma()))
        else:
            # Voigt model (Gaussian and Lorentzian)
            from scipy.special import wofz
            w = wofz(((x - self.mu()) + 1j*self.gamma())* 2**-0.5/self.sig())
            voigt_result = self.baseline() + (self.A() * w.real*(2*np.pi)**-0.5/self.sig())
        return voigt_result

    def fitData(self, x, y, weights=None, parinfo=None):
        if len(parinfo) != 5:
            raise Exception("Wrong number of parameters!")
        super(VoigtModel, self).fitData(x, y, weights, parinfo)


    def baseline(self): return self._parinfo[0]['value']
    def A(self): return self._parinfo[1]['value']
    def sig(self): return self._parinfo[2]['value']
    def mu(self): return self._parinfo[3]['value']
    def gamma(self): return self._parinfo[4]['value']

    def _make_voigt(self):
        if self.sig == 0:
            # Equivalent to a Lorentzian model
            k = self.A()*self.gamma()/np.pi
            s = self.mu()*self.mu()+self.gamma()*self.gamma()
            def f(x):
                return k/(x*x - 2*x*self.mu()+s)
        else:
            # Voigt model (Gaussian and Lorentzian)
            k = self.A() * (2*np.pi)**-0.5/self.sig()
            s = 2**-0.5/self.sig()
            def f(x):
                from scipy.special import wofz
                return k * wofz(((x - self.mu()) + 1j*self.gamma())*s).real

        return f

    def integrate(self, from_x=None, to_x=None):
        # Define range: Include 99.97% of the gaussian area
        if from_x is None:
            from_x = self.mu() - 3*self.sig()
        if to_x is None:
            to_x = self.mu() + 3*self.sig()

        #if self.x is None:
            #return 0
        #else:
        from scipy.integrate import quad
        integral, estimated_error = quad(self._make_voigt(), from_x, to_x)
        return integral

    # Returns fwhm in nm and kms
    def fwhm(self):
        # FWHM for voigt
        # http://en.wikipedia.org/wiki/Voigt_profile#The_width_of_the_Voigt_profile

        # Light speed in vacuum
        c = 299792458.0 # m/s
        fwhm_gaussian = self.sig() * (2*np.sqrt(2*np.log(2))) # nm
        fwhm_lorentzian = 2*self.gamma()
        phi = fwhm_lorentzian / fwhm_gaussian

        c0 = 2.0056
        c1 = 1.0593
        fwhm = fwhm_gaussian * (1 - c0*c1 + np.sqrt(np.power(phi, 2) + 2*c1*phi + c0*c0*c1*c1)) # nm
        fwhm_kms = (c / (self.mu() / fwhm)) / 1000.0 # km/s
        return fwhm, fwhm_kms

    def resolution(self):
        fwhm, fwhm_kms = self.fwhm()
        return self.mu() / fwhm

    # Returns fwhm in nm and kms
    def fwhm_olivero(self):
        # FWHM for voigt
        # Formula from Olivero et al (1977) "Empirical fits to the Voigt line width: A brief review"
        # http://www.sciencedirect.com/science/article/pii/0022407377901613
        # http://en.wikipedia.org/wiki/Voigt_profile#The_width_of_the_Voigt_profile

        # Light speed in vacuum
        c = 299792458.0 # m/s
        fwhm_gaussian = self.sig() * (2*np.sqrt(2*np.log(2))) # nm
        fwhm_lorentzian = 2*self.gamma()

        fwhm = 0.5346*(2*fwhm_lorentzian) + np.sqrt(0.2166*np.power(fwhm_lorentzian, 2) + np.power(fwhm_gaussian, 2)) # nm
        fwhm_kms = (c / (self.mu() / fwhm)) / 1000.0 # km/s
        return fwhm, fwhm_kms

    def resolution_olivero(self):
        fwhm, fwhm_kms = self.fwhm_olivero()
        return self.mu() / fwhm




if __name__ == '__main__':
    ### Full range
    x = np.arange(-10.,10., 1)
    ### Reduced range
    ##x = np.arange(-3.,3., 0.5)

    x_fine = np.arange(-10.,10., 20./1000)

    ############### GAUSSIAN
    ## Generate model data for a Gaussian with param mu and sigma and add noise
    baseline = 10
    A = -1
    sig = 2
    mu = 0.5
    gaussian_model = GaussianModel(baseline, A, sig, mu)
    y_true = gaussian_model(x)

    #preal = [-1, -2, .5]
    #y_true = gaussian_model(x, preal)

    mu, sigma = 0, 0.7
    y      = y_true + 0.01 * np.random.normal(mu, sigma, len(x) )
    ##err    = 1.0 + 0.01 * np.random.normal(mu, sigma, len(x) )
    err = np.ones(len(x))

    gaussian_model.fitData(x, y)


    print "Fitted pars: "
    print "\tBase:\t", gaussian_model.baseline()
    print "\tA:\t", gaussian_model.A()
    print "\tsig:\t", gaussian_model.sig()
    print "\tmu:\t", gaussian_model.mu()
    fwhm, fwhm_kms = gaussian_model.fwhm()
    R = gaussian_model.resolution()
    print "FWHM: ", fwhm
    print "FWHM km/s: ", fwhm_kms
    print "R: ", R
    print "RMS: ", np.sqrt(np.sum(np.power(gaussian_model.residuals(), 2)) / len(gaussian_model.residuals()))

    plt.clf()
    plt.plot(x,y,'r', label="Noisy data")
    plt.plot( x_fine, gaussian_model(x_fine), label="Fit" )
    plt.plot( x, y_true, 'g', label="True data" )
    plt.xlabel( "X" )
    plt.ylabel( "Measurement data" )
    plt.title( "Least-squares fit to noisy data using MPFIT" )
    plt.legend()
    plt.show()

    ############### VOIGT
    ## Generate model data for a Gaussian with param mu and sigma and add noise
    baseline = 10
    A = -1
    sig = 2
    mu = 0.5
    gamma = 0.5
    voigt_model = VoigtModel(baseline, A, sig, mu, gamma)
    y_true = voigt_model(x)

    #preal = [-1, -2, .5]
    #y_true = gaussian_model(x, preal)

    mu, sigma = 0, 0.7
    y      = y_true + 0.01 * np.random.normal(mu, sigma, len(x) )
    ##err    = 1.0 + 0.01 * np.random.normal(mu, sigma, len(x) )
    err = np.ones(len(x))

    voigt_model.fitData(x, y)

    print "Fitted pars: "
    print "\tBase:\t", voigt_model.baseline()
    print "\tA:\t", voigt_model.A()
    print "\tsig:\t", voigt_model.sig()
    print "\tmu:\t", voigt_model.mu()
    print "\tgamma:\t", voigt_model.gamma()
    fwhm, fwhm_kms = voigt_model.fwhm()
    R = voigt_model.resolution()
    print "FWHM: ", fwhm
    print "FWHM km/s: ", fwhm_kms
    print "R: ", R
    fwhm, fwhm_kms = voigt_model.fwhm_olivero()
    R = voigt_model.resolution_olivero()
    print "* Olivero version *"
    print "FWHM: ", fwhm
    print "FWHM km/s: ", fwhm_kms
    print "R: ", R
    print "*******************"
    print "RMS: ", np.sqrt(np.sum(np.power(voigt_model.residuals(), 2)) / len(voigt_model.residuals()))

    plt.clf()
    plt.plot(x,y,'r', label="Noisy data")
    plt.plot( x_fine, voigt_model(x_fine), label="Fit" )
    plt.plot( x, y_true, 'g', label="True data" )
    plt.xlabel( "X" )
    plt.ylabel( "Measurement data" )
    plt.title( "Least-squares fit to noisy data using MPFIT" )
    plt.legend()
    plt.show()


