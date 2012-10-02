/**
    This file is part of Spectra.
    Copyright 2011-2012 Sergi Blanco Cuaresma - http://www.marblestation.com
    
    Spectra is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spectra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with Spectra.  If not, see <http://www.gnu.org/licenses/>.
**/

typedef void (*progressfunc)(double num, void *user_data);

int synthesize_spectrum(char *atmosphere_model_file, char *linelist_file, char *abundances_file, double microturbulence_vel, int verbose, int num_measures, const double waveobs[], double fluxes[], progressfunc user_func, void *user_data);

int macroturbulence_spectrum(const double waveobs[], double fluxes[], int num_measures, double macroturbulence, int verbose, progressfunc user_func, void *user_data);

int rotation_spectrum(const double waveobs[], double fluxes[], int num_measures, double vsini, double limb_darkening_coeff, int verbose, progressfunc user_func, void *user_data);

int resolution_spectrum(const double waveobs[], double fluxes[], int num_measures, int R, int verbose, progressfunc user_func, void *user_data);

int abundances_determination(char *atmosphere_model_file, char *linelist_file, int num_measures, char *abundances_file, double microturbulence_vel, int verbose, double abundances[], double normal_abundances[], double relative_abundances[], progressfunc user_func, void *user_data);
