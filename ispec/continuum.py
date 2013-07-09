#
#    This file is part of the Integrated Spectroscopic Framework (iSpec).
#    Copyright 2011-2012 Sergi Blanco Cuaresma - http://www.marblestation.com
#
#    iSpec is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    iSpec is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with iSpec. If not, see <http://www.gnu.org/licenses/>.
#
import numpy as np
from plotting import *
from common import *
from spectrum import *
from pymodelfit import UniformCDFKnotSplineModel
from scipy import interpolate
import log
import logging

def read_continuum_regions(continuum_regions_filename):
    """
    Read continuum regions.
    The specified file should be plain text with **tab** character as column delimiter.
    Two columns should exists: 'wave_base' and 'wave_top' (the first line should contain those header names).
    They indicate the beginning and end of each region (one per line). For instance:
    ::

        wave_base       wave_top
        480.6000        480.6100
        481.1570        481.1670
        491.2240        491.2260
        492.5800        492.5990
    """

    continuum_regions = np.array([tuple(cont.rstrip('\r\n').split("\t")) for cont in open(continuum_regions_filename,)][1:], dtype=[('wave_base', float),('wave_top', float)])

    if np.any(continuum_regions['wave_top'] - continuum_regions['wave_base'] <= 0):
        logging.error("Continuum regions where wave_base is equal or bigger than wave_top")
        raise Exception("Incompatible format")

    return continuum_regions

def write_continuum_regions(continuum_regions, continuum_regions_filename):
    """
    Write continuum regions file with the following format:
    ::

        wave_base       wave_top
        480.6000        480.6100
        481.1570        481.1670
        491.2240        491.2260
        492.5800        492.5990
    """
    out = open(continuum_regions_filename, "w")
    out.write("wave_base\twave_top\n")
    out.write("\n".join(["\t".join(map(str, (cont['wave_base'], cont['wave_top']))) for cont in continuum_regions]))
    out.close()

def __find_max_value_per_wavelength_range(spectrum, base_points, wave_range=1):
    """
    Group the points in ranges of 1 nm (by default) and select the one with the maximum flux.
    """
    return __find_a_value_per_wavelength_range(spectrum, base_points, wave_range=wave_range, median=False)

def __find_median_value_per_wavelength_range(spectrum, base_points, wave_range=1):
    """
    Group the points in ranges of 1 nm (by default) and select the one with the median flux.
    """
    return __find_a_value_per_wavelength_range(spectrum, base_points, wave_range=wave_range, median=True)

def __find_a_value_per_wavelength_range(spectrum, base_points, wave_range=1, median=False):
    """
    Group the points in ranges of 1 nm (by default) and select the one with the maximum (by default) or median flux.
    """
    waveobs = spectrum['waveobs']
    flux = spectrum['flux']
    wave_step = wave_range # nm
    wave_base = np.min(waveobs)
    wave_top = np.max(waveobs)

    # Group points in bins and use only the one with the higher flux
    num_candidate_base_points = int((wave_top-wave_base)/wave_step) + 1
    candidate_base_points = -9999 * np.ones(num_candidate_base_points, dtype=int)
    i = 0
    while wave_base < wave_top:
        positions = np.where((waveobs[base_points] >= wave_base) & (waveobs[base_points] <= wave_base + wave_step))[0]

        if len(positions) == 0:
            candidate_base_points[i] = -9999
        else:
            if median:
                # Find the median (instead of the max)
                f = flux[base_points[positions]]
                sortedi = np.argsort(np.abs(f - np.median(f))) # The smallest is the median (this is the best way to avoid floating-point imprecisions)
                candidate_base_points[i] = base_points[positions[sortedi[0]]]
            else:
                # Find max for this bin
                sortedi = np.argsort(flux[base_points[positions]])
                candidate_base_points[i] = base_points[positions[sortedi[-1]]]

        #ipdb.set_trace()
        wave_base += wave_step
        i += 1

    candidate_base_points = candidate_base_points[candidate_base_points != -9999]
    return candidate_base_points


def __discard_outliers_for_continuum_candidates(spectrum, candidate_base_points, sig=3):
    """
    Considering the diference in flux of the points with the next a previous, discard outliers (iterative process).
    Median value and 3*sigma is used as criteria for outliers detection.
    """
    # The change between consecutive base points for continuum fitting should not be very strong,
    # identify outliers (first and last base point are excluded in this operation):
    flux_diff1 = (spectrum['flux'][candidate_base_points][:-1] - spectrum['flux'][candidate_base_points][1:]) / (spectrum['waveobs'][candidate_base_points][:-1] - spectrum['waveobs'][candidate_base_points][1:])
    flux_diff2 = (spectrum['flux'][candidate_base_points][1:] - spectrum['flux'][candidate_base_points][:-1]) / (spectrum['waveobs'][candidate_base_points][1:] - spectrum['waveobs'][candidate_base_points][:-1])
    # Recover first and last
    flux_diff1 = np.asarray(flux_diff1.tolist() + [flux_diff2[-1]])
    flux_diff2 = np.asarray([flux_diff1[0]] + flux_diff2.tolist())
    # Identify outliers
    #flux_diff1_selected, not_outliers1 = sigma_clipping(flux_diff1, sig=sig, meanfunc=np.median)
    #flux_diff2_selected, not_outliers2 = sigma_clipping(flux_diff2, sig=sig, meanfunc=np.median)
    flux_diff1_selected, not_outliers1 = interquartile_range_filtering(flux_diff1, k=1.5)
    flux_diff2_selected, not_outliers2 = interquartile_range_filtering(flux_diff2, k=1.5)
    outliers = np.logical_or(np.logical_not(not_outliers1), np.logical_not(not_outliers2))

    # Ensure that first and last points are not filtered out in order to avoid
    # having strange extrapolations in the edges of the spectrum because lack of points in the fit
    outliers = np.asarray([False] + outliers[1:-1].tolist() + [False])
    # Discard outliers
    continuum_base_points = candidate_base_points[~outliers]

    return continuum_base_points


def __determine_continuum_base_points(spectrum, discard_outliers=True, median_wave_range=0.1, max_wave_range=1):
    """
    Determine points to be used for continuum fitting by following these steps:

    1) Determine max points by using a moving window of 3 elements (also used for line determination).
    2) Group the points:
        - In ranges of 0.1 nm and select the one with the median flux (usefull to avoid noisy peaks).
        - In ranges of 1 nm and select the one with the max flux.
    3) Considering the diference in flux of the points with the next a previous, discard outliers (iterative process).
    """
    # Find max points in windows of 3 measures
    candidate_base_points = find_local_max_values(spectrum['flux'])
    if median_wave_range > 0:
        candidate_base_points = __find_median_value_per_wavelength_range(spectrum, candidate_base_points, wave_range=median_wave_range)
    if max_wave_range > 0:
        candidate_base_points = __find_max_value_per_wavelength_range(spectrum, candidate_base_points, wave_range=max_wave_range)
    if discard_outliers:
        candidate_base_points = __discard_outliers_for_continuum_candidates(spectrum, candidate_base_points)
    continuum_base_points = candidate_base_points


    return continuum_base_points

def fit_continuum(spectrum, independent_regions=None, continuum_regions=None, nknots=None, median_wave_range=0.1, max_wave_range=1, fixed_value=None, model='Polynomy'):
    #nknots = 2
    #median_wave_range = 0.05
    #max_wave_range = 0.2

    if independent_regions is not None:
        if len(independent_regions) == 0:
            raise Exception("No segments defined!")

        wave_step = 0.0001
        xaxis = np.arange(spectrum['waveobs'][0], spectrum['waveobs'][-1]+wave_step, wave_step)
        #fluxes = np.ones(len(xaxis))
        fluxes = np.zeros(len(xaxis))
        num_success = 0
        for i, region in enumerate(independent_regions):
            wfilter = np.logical_and(spectrum['waveobs'] >= region['wave_base'], spectrum['waveobs'] <= region['wave_top'])
            try:
                if len(spectrum[wfilter]) > 10:
                    continuum = __fit_continuum(spectrum[wfilter], continuum_regions=continuum_regions, nknots=nknots, median_wave_range=median_wave_range, max_wave_range=max_wave_range, fixed_value=fixed_value, model=model)
                    # Save
                    wfilter = np.logical_and(xaxis >= region['wave_base'], xaxis <= region['wave_top'])
                    fluxes[np.where(wfilter)[0]] = continuum(xaxis[wfilter])
                    num_success += 1
            except:
                print "Continuum fit failed for segment #", i, "[", region['wave_base'], ",", region['wave_top'], "]"
                pass
        if num_success == 0:
            raise Exception("Impossible to fit continuum to any of the segments")

        #continuum = interpolate.InterpolatedUnivariateSpline(xaxis, fluxes, k=3)
        continuum = interpolate.interp1d(xaxis, fluxes, kind='linear', bounds_error=False, fill_value=0.0)
    else:
        continuum = __fit_continuum(spectrum, continuum_regions=continuum_regions, nknots=nknots, median_wave_range=median_wave_range, max_wave_range=max_wave_range, fixed_value=fixed_value, model=model)
    return continuum


def __fit_continuum(spectrum, continuum_regions=None, nknots=None, median_wave_range=0.1, max_wave_range=1, fixed_value=None, model='Polynomy'):
    """
    If fixed_value is specified, the continuum is fixed to the given value (always
    the same for any wavelength). If not, fit the continuum by following these steps:

    1) Determine continuum base points:
        a. Find base points by selecting local max. values (3 points).
        b. Find the median value per each 0.1 nm (avoid noisy peaks).
        c. Find the max. value per each 1 nm (avoid blended base points).
        d. Discard outliers considering the median +/- 3 x sigma.
    2) Fitting (depending model value):
        1. Fixed value
        2. Spline fitting:
            a. The number of splines can be specified, if not it will use 1 spline every 10 nm.
            b. The algorithm automatically distributes and assigns more splines to regions more populated with base points.
            c. If there are not enough data points to fit, the whole process is repeated but without discarding outliers.
        3. Polynomial fitting
    3) Returns the fitted model.
    """
    if not model in ['Splines', 'Polynomy', 'Fixed value']:
        raise Exception("Wrong model name!")

    if model == 'Fixed value' and fixed_value is None:
        raise Exception("Fixed value needed!")

    class ConstantValue:
        def __init__(self, value):
            self.value = value

        def __call__(self, x):
            try:
                return np.asarray([self.value] * len(x))
            except TypeError:
                # It's not an array, return a single value
                return self.value

    if model == 'Fixed value':
        return ConstantValue(fixed_value)

    if continuum_regions is not None:
        spectrum_regions = None
        for region in continuum_regions:
            wave_filter = (spectrum['waveobs'] >= region['wave_base']) & (spectrum['waveobs'] <= region['wave_top'])
            new_spectrum_region = spectrum[wave_filter]
            if spectrum_regions is None:
                spectrum_regions = new_spectrum_region
            else:
                spectrum_regions = np.hstack((spectrum_regions, new_spectrum_region))
        spectrum = spectrum_regions

    continuum_base_points = __determine_continuum_base_points(spectrum, discard_outliers=True, median_wave_range=median_wave_range, max_wave_range=max_wave_range)

    if nknots is None:
        # * 1 knot every 10 nm in average
        nknots = np.max([1, int((np.max(spectrum['waveobs']) - np.min(spectrum['waveobs'])) / 10)])

    if len(spectrum['waveobs'][continuum_base_points]) == 0:
        raise Exception("Not enough points to fit")

    # UniformCDFKnotSplineModel:
    # 1) Builds an histogram: # points in 2*nknots bins
    #    - cdf: Number of points in that bin
    #    - xcdf[n], xcdf[n+1]: Limits of a bin in wavelength
    # 2) Filter bins with no points
    # 3) Calculate the cumulative sum of # points and normalize it
    # 4) Interpolate the wavelength in an homogeneus grid of normalized cumulative sum of points
    #    (from 0 to 1, divided in nknots)
    #    - x=0.02 means wavelength where we reach the 2% of the total number of points
    #    - x=0.98 means wavelength where we reach the 98% of the total number of points
    # 5) Use those interpolated wavelengths for putting the knots, therefore we will have a knot
    #    in those regions were there are an increment on the number of points (avoiding empty regions)


    continuum_model = UniformCDFKnotSplineModel(nknots)
    fitting_error = False
    try:
        if model == "Splines":
            continuum_model.fitData(spectrum['waveobs'][continuum_base_points], spectrum['flux'][continuum_base_points])
        else:
            continuum_model = np.poly1d(np.polyfit(spectrum['waveobs'][continuum_base_points], spectrum['flux'][continuum_base_points], nknots))
    except Exception, e:
        ipdb.set_trace()
        fitting_error = True

    # If there is no fit (because too few points)
    if fitting_error or ("residuals" in dir(continuum_model) and np.any(np.isnan(continuum_model.residuals()))):
        # Try without discarding outliers:
        continuum_base_points = __determine_continuum_base_points(spectrum, discard_outliers=False, median_wave_range=median_wave_range,
max_wave_range=max_wave_range)
        if model == "Splines":
            continuum_model.fitData(spectrum['waveobs'][continuum_base_points], spectrum['flux'][continuum_base_points])
        else:
            continuum_model = np.poly1d(np.polyfit(spectrum['waveobs'][continuum_base_points], spectrum['flux'][continuum_base_points], nknots))
        if np.any(np.isnan(continuum_model.residuals())):
            raise Exception("Not enough points to fit")

    return continuum_model


def find_continuum(spectrum, resolution, segments=None, max_std_continuum = 0.002, continuum_model = 0.95, max_continuum_diff=0.01, fixed_wave_step=None, frame=None):
    """
    Find regions of wavelengths where the fluxes seem to belong to the continuum:

    - The region size is variable in function of 4*fwhm which is derived
      from the current wavelength and the resolution (unless a fixed_wave_step is specified)
    - A region is accepted as continuum if the following criteria is true:

        a) the median flux is above the continuum model (but not more than 0.08) or below but not more than 0.01
        b) and the standard deviation is less than a given maximum the region is selected
    - If 'segments' is specified, then the search is limited to that wavelengths areas
    """
    min_wave = np.min(spectrum['waveobs'])
    max_wave = np.max(spectrum['waveobs'])

    last_reported_progress = -1
    total_work_progress = max_wave - min_wave
    if frame is not None:
        frame.update_progress(0)

    if segments is None:
        # Use whole spectrum
        segments = np.array([(min_wave, max_wave)], dtype=[('wave_base', float),('wave_top', float)])

    dirty_continuum_regions = []

    for segment in segments:
        wave_base = segment['wave_base']
        if fixed_wave_step is not None:
            wave_increment = fixed_wave_step
        else:
            wave_increment = (wave_base / resolution) * 4
        wave_top = wave_base + wave_increment

        i = 0
        max_limit = segment['wave_top']
        while (wave_top < max_limit):
            #~ print wave_base, ">"
            # Filter values that belong to the wavelength segment
            wave_filter = (spectrum['waveobs'] >= wave_base) & (spectrum['waveobs'] < wave_top)
            # Stats for current segment
            spectrum_region = spectrum[wave_filter]
            mean_flux = np.mean(spectrum_region['flux'])
            median_flux = np.median(spectrum_region['flux'])
            std_flux = np.std(spectrum_region['flux'])
            num_measures = len(spectrum_region['flux'])

            # Continuum_model can be a fitted model or a fixed number
            if isinstance(continuum_model, float) or isinstance(continuum_model, int):
                cont_diff = median_flux - continuum_model
                cont_diff_percentage = np.abs(cont_diff) / continuum_model
            else:
                c = continuum_model(wave_base)
                cont_diff = median_flux - c
                cont_diff_percentage = np.abs(cont_diff) / c
            # Flux should be above the continuum model but no more than a given limit (1% by default)
            near_continuum = (cont_diff_percentage <= max_continuum_diff)

            if (num_measures > 0 and std_flux < max_std_continuum and near_continuum):
                last_accepted = True
                dirty_continuum_regions.append((wave_base, wave_top, num_measures, mean_flux, std_flux))
            else:
                last_accepted = False
                #~ print "Discarded (std = " + str(std_flux) + ", mean = " + str(mean_flux) + ", cont_diff=" + str(cont_diff) + ")"

            # Go to next region
            wave_base = wave_top
            if fixed_wave_step is not None:
                wave_increment = fixed_wave_step
            else:
                wave_increment = (wave_base / resolution) * 4
            if not last_accepted:
                wave_increment = wave_increment / 2 # Half increase
            wave_top = wave_base + wave_increment


            current_work_progress = ((wave_base - min_wave) / total_work_progress) * 100
            if report_progress(current_work_progress, last_reported_progress):
                last_reported_progress = current_work_progress
                logging.info("%.2f%%" % current_work_progress)
                if frame is not None:
                    frame.update_progress(current_work_progress)

            i += 1

    continuum_regions = np.array(dirty_continuum_regions,  dtype=[('wave_base', float), ('wave_top', float), ('num_measures', int), ('mean_flux', float), ('std_flux', float)])

    continuum_regions = __merge_regions(spectrum, continuum_regions)
    logging.info("Found %i continuum regions" % len(continuum_regions))

    return continuum_regions



def __merge_regions(spectrum, dirty_continuum_regions):
    """
    Given a group of continuum regions of a spectrum, merge those that are
    consecutive.
    """
    ### It can happend that consecutives regions with different mean_increase are
    ### selected to belong to the continuum. We can merge them for coherence:
    cleaned_continuum_regions = []
    i = 0
    # For all regions (except the last one), check the next one is consecutive in wavelengths
    while i < len(dirty_continuum_regions) - 2:
        j = 0
        # While wave_top of the current is equal to wave_base of the next...
        while ((dirty_continuum_regions[j+i]['wave_top'] == dirty_continuum_regions[j+i+1]['wave_base']) and (j < len(dirty_continuum_regions) - 2 - i)):
            j += 1

        wave_base = dirty_continuum_regions[i]['wave_base']
        wave_top = dirty_continuum_regions[j+i]['wave_top']

        if j == 1: # No merge needed
            num_measures = dirty_continuum_regions[i]['num_measures']
            mean_flux = dirty_continuum_regions[i]['mean_flux']
            std_flux = dirty_continuum_regions[i]['std_flux']
        else:      # Merge and calculate new stats
            wave_filter = (spectrum['waveobs'] >= wave_base) & (spectrum['waveobs'] < wave_top)
            mean_flux = np.mean(spectrum['flux'][wave_filter])
            std_flux = spectrum['flux'][wave_filter].std()
            num_measures = len(spectrum['flux'][wave_filter])

        cleaned_continuum_regions.append((wave_base, wave_top, num_measures, mean_flux, std_flux))
        i += j + 1 # Skip the regions that have been merged

    # Convert result array to numpy array
    continuum_regions = np.array(cleaned_continuum_regions,  dtype=[('wave_base', float), ('wave_top', float), ('num_measures', int), ('mean_flux', float), ('std_flux', float)])

    return continuum_regions



