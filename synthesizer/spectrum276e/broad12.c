#include <math.h>
#include "spectrum.h"
#include <string.h>
#include <errno.h>
int approx();
double gammln();
int sp();
int pd();
int df();

void broad(atmosphere * model, linedata * line, int N, double Sig, double Alp, double fac)
{
    double charge, C6, C64, C62, v, gammar, gammaw, gammas, cneutral, neffl, neffh;
    double neffh5;
    int i, ll, lh, flago = 0;
    int flagS = 0;
    int flagSig = 0;
    int flagAlp = 0;
    static double pi = 3.141592654;
    static double c = 2.997924562e+18;
    double E = 2.0;
    extern int Ntau;
    extern int flagu;
    double vbar, sig, alp, OMara, Rl2, Rh2, lnwN;
    double vturb;

    if (strcmp(line[N].T, "AI") == 0) {
        for (i = 0; i < Ntau; i++) {
            vturb = model->mtv[i];
            line[N].dopp[i] = sqrt(1.6631e+08 * model->T[i] / line[N].atomass + (vturb * vturb));
        }
        return;
    }

    flagS = 0;

    /* Check to see if Sigma and Alpha have been specified in the line list */

    if (approx(Sig, 0.000, 0.01) == 1)
        flagSig = 0;
    else
        flagSig = 1;

    if (approx(Alp, 0.000, 0.01) == 1)
        flagAlp = 0;
    else
        flagAlp = 1;

    OMara = C64 = 0.0;
    if (flagSig != 0 && flagAlp != 0) {
        flagS = 1;
        flago = 1;
        sig = Sig;
        alp = Alp;
    }

    if (line[N].code < 100.0) {
        charge = 1.0;
        cneutral = floor(line[N].code);
        if (approx(line[N].code, cneutral, 0.001) == 1)
            charge = 1.0;
        else if (approx(line[N].code, cneutral + 0.1, 0.001) == 1)
            charge = 2.0;
        else if (approx(line[N].code, cneutral + 0.2, 0.001) == 1)
            charge = 3.0;
        else if (approx(line[N].code, cneutral + 0.3, 0.001) == 1)
            charge = 4.0;
        else
            charge = 2.0;

        /* Define neffl and neffh and fudge things for autoionizing lines */
        if (line[N].chi <= line[N].El)
            neffl = 4.0;
        else
            neffl = charge * sqrt(13.595 / (line[N].chi - line[N].El));
        if (line[N].chi <= line[N].Eu)
            neffh = 5.0;
        else
            neffh = charge * sqrt(13.595 / (line[N].chi - line[N].Eu));
        if (neffh <= neffl)
            neffl = neffh - 1.0;

        /* printf("%10.3f neffl = %4.2f neffh =
           %4.2f\n",line[N].wave,neffl,neffh); */

        /* Define type of transition and determine ll and lh */
        if (strcmp(line[N].T, "99") == 0) {
            ll = 9;
            lh = 9;
            flago = 3;
        } else if (strcmp(line[N].T, "01") == 0) {
            ll = 0;
            lh = 1;
            if (flagS == 0)
                flago = sp(neffl, neffh, ll, lh, &sig, &alp, charge);
        } else if (strcmp(line[N].T, "10") == 0) {
            ll = 1;
            lh = 0;
            if (flagS == 0)
                flago = sp(neffl, neffh, ll, lh, &sig, &alp, charge);
            /* printf("sig = %f alp = %f\n",sig,alp); */
        } else if (strcmp(line[N].T, "12") == 0) {
            ll = 1;
            lh = 2;
            if (flagS == 0)
                flago = pd(neffl, neffh, ll, lh, &sig, &alp, charge);
        } else if (strcmp(line[N].T, "21") == 0) {
            ll = 2;
            lh = 1;
            if (flagS == 0)
                flago = pd(neffl, neffh, ll, lh, &sig, &alp, charge);
        } else if (strcmp(line[N].T, "23") == 0) {
            ll = 2;
            lh = 3;
            if (flagS == 0)
                flago = df(neffl, neffh, ll, lh, &sig, &alp, charge);
        } else if (strcmp(line[N].T, "32") == 0) {
            ll = 3;
            lh = 2;
            if (flagS == 0)
                flago = df(neffl, neffh, ll, lh, &sig, &alp, charge);
        } else if (strcmp(line[N].T, "34") == 0) {
            ll = 3;
            lh = 4;
            flago = 2;
        } else if (strcmp(line[N].T, "43") == 0) {
            ll = 4;
            lh = 3;
            flago = 2;
        } else if (strcmp(line[N].T, "45") == 0) {
            ll = 4;
            lh = 5;
            flago = 2;
        } else if (strcmp(line[N].T, "54") == 0) {
            ll = 5;
            lh = 4;
            flago = 2;
        } else if (strcmp(line[N].T, "56") == 0) {
            ll = 5;
            lh = 6;
            flago = 2;
        } else if (strcmp(line[N].T, "65") == 0) {
            ll = 6;
            lh = 5;
            flago = 2;
        } else if (strcmp(line[N].T, "AO") == 0) {
            ll = 0;
            lh = 1;
            flago = 1;
        } else if (strcmp(line[N].T, "GA") == 0) {
            ll = 0;
            lh = 1;
            flago = 4;
            /* Next lines take into account cases where broadening widths are
               entered in line list as 0.00 */
            if (approx(line[N].gammaw, 1.0, 0.001) == 1)
                flago = 5;
            if (approx(line[N].gammas, 1.0, 0.001) == 1 && flago == 5)
                flago = 6;
            if (approx(line[N].gammar, 1.0, 0.001) == 1 && flago == 6)
                flago = 7;
        } else {
            ll = 9;
            lh = 9;
            flago = 3;
        }

        if (flago == 1)
            OMara = (alp / 2.0) * 0.24156448 + gammln((4.0 - alp) / 2.0) + log(sig);
        if (flago == 0 || flago == 2) {
            Rl2 = neffl * neffl * (5.0 * neffl * neffl + 1.0 - 3.0 * ll * (ll + 1.0)) / (2.0 * charge * charge);
            Rh2 = neffh * neffh * (5.0 * neffh * neffh + 1.0 - 3.0 * lh * (lh + 1.0)) / (2.0 * charge * charge);
            C6 = 4.05e-33 * (Rh2 - Rl2);
            C62 = pow(C6, 0.2);
            C64 = pow(C6, 0.4);
        }
        if (flago == 3 || flago == 5 || flago == 6 || flago == 7) {
            Rl2 = neffl * neffl * (5.0 * neffl * neffl) / (2.0 * charge * charge);
            Rh2 = neffh * neffh * (5.0 * neffh * neffh) / (2.0 * charge * charge);
            C6 = 4.05e-33 * (Rh2 - Rl2);
            C62 = pow(C6, 0.2);
            C64 = pow(C6, 0.4);
        }

        neffh5 = pow(neffh, 5.0);

        /* Radiation broadening */
        /*if (flago == 4 || flago == 5 || flago == 6)*/
        if (flago == 4 || flago == 5 || flago == 6 || (flago == 1 && line[N].gammar != 0)) // SBC
            gammar = line[N].gammar;
        else
            gammar = 2.223e+15 / (line[N].wave * line[N].wave);

        // SBC
        /*if (flago == 1){*/
            /*if (line[N].gammar == 0) {*/
                /*printf("line[N].gammar == 0\n");*/
            /*}*/
            /*printf("With parameter :: %f :: gammar = %f\n", line[N].wave, line[N].gammar);*/
            /*printf("With formula   :: %f :: gammar = %f\n", line[N].wave, 2.223e+15 / (line[N].wave * line[N].wave));*/
        /*}*/

        for (i = 0; i < Ntau; i++) {
            vturb = model->mtv[i];
            line[N].dopp[i] = sqrt(1.6631e+08 * model->T[i] / line[N].atomass + (vturb * vturb));

            /* Van Der Waals Broadening */
            if (flago == 4)
                gammaw = line[N].gammaw * model->NHI[i] * pow(model->T[i] / 10000.0, 0.3) * (1.0 + 0.4133 * model->NHeI[i] / model->NHI[i] + 0.85 * model->NH2[i] / model->NHI[i]);
            else {
                v = sqrt(2.1175e+08 * model->T[i] * (1.0 / line[N].atomass + 0.9921));
                if (flago == 1) {
                    /*lnwN = OMara + log(v) + (1.0 - alp) * log(v / 1.0e+06);*/
                    lnwN = OMara + log(1.0e6) + (1.0 - alp) * log(v / 1.0e+06); // SBC: v0 = 1.0e6
                    gammaw = 2.0 * fac * 2.8003e-17 * exp(lnwN) * model->NHI[i] * (1.0 + 0.4133 * model->NHeI[i] / model->NHI[i] + 0.85 * model->NH2[i] / model->NHI[i]);
                } else
                    gammaw = 8.08 * fac * C64 * pow(v, 0.6) * model->NHI[i] * (1.0 + 0.4133 * model->NHeI[i] / model->NHI[i] + 0.85 * model->NH2[i] / model->NHI[i]);

                // SBC
                /*if (flago == 1 && i == 0){*/
                    /*printf("AO :: %f :: gammaw = %f\n", line[N].wave, gammaw);*/
                    /*printf("GA :: %f :: gammaw = %f\n", line[N].wave, pow(10.0, -7.675) * model->NHI[i] * pow(model->T[i] / 10000.0, 0.3) * (1.0 + 0.4133 * model->NHeI[i] / model->NHI[i] + 0.85 * model->NH2[i] / model->NHI[i]));*/
                /*}*/

                /* The multiplicative term (1 + ...) in the above equation
                   takes into account the broadening by helium and H2.  In the 
                   following equation dlg is calculated in order to apply a
                   first-order correction term to Van Der Waals broadening to
                   take into account quasistatic broadening This expression
                   disabled and set equal to 1.0 Oct 3, 2001.  See note in
                   strong3.c */
                /* line[N].dlg[i] = ((line[N].wave*line[N].wave)/(2.0*pi*c))*
                   pow(v,1.2)/(1.217355*C62); */
                line[N].dlg[i] = 1.0;
            }

            /* Quadratic Stark Broadening */
            /*if (flago == 4 || flago == 5)*/
            if (flago == 4 || flago == 5 || (flago == 1 && line[N].gammas != 0)) // SBC
                gammas = line[N].gammas * model->Ne[i];
            else
                gammas = 1.0e-08 * neffh5 * model->Ne[i];
            
            // SBC
            /*if (flago == 1 && i == 0){*/
                /*if (line[N].gammas == 0) {*/
                    /*printf("line[N].gammas == 0\n");*/
                /*}*/
                /*printf("With parameter :: %f :: gammas = %f\n", line[N].wave, line[N].gammas * model->Ne[i]);*/
                /*printf("With formula   :: %f :: gammas = %f\n", line[N].wave, 1.0e-08 * neffh5 * model->Ne[i]);*/
            /*}*/

            line[N].a[i] = (gammar + gammaw + gammas) * line[N].wave * 1.0e-08 / (12.5636 * line[N].dopp[i]);
        }
        return;
    } else {
        /* Approximate broadening parameters for molecules */

        gammar = 2.223e+15 / (line[N].wave * line[N].wave);
        for (i = 0; i < Ntau; i++) {
            vturb = model->mtv[i];
            line[N].dopp[i] = sqrt(1.6631e+08 * model->T[i] / line[N].atomass + (vturb * vturb));
            gammaw = 1.0e-07 * model->NHI[i];
            gammas = 1.0e-05 * model->Ne[i];
            line[N].a[i] = (gammar + gammaw + gammas) * line[N].wave * 1.0e-08 / (12.5636 * line[N].dopp[i]);
        }
        return;
    }
}

int sp(neffl, neffh, ll, lh, s, a, charge)
double neffl, neffh, charge;
double *s, *a;
int ll, lh;
{
    static float sig[21][18] = {
        {126, 140, 165, 202, 247, 299, 346, 383, 435, 491, 553, 617,
         685, 769,
         838, 925, 1011, 1082},
        {140, 150, 162, 183, 218, 273, 327, 385, 440, 501, 557, 620,
         701, 764,
         838, 923, 1025, 1085},
        {154, 167, 175, 192, 216, 251, 299, 357, 423, 487, 549, 617,
         684, 759,
         834, 910, 1014, 1064},
        {166, 180, 192, 206, 226, 253, 291, 339, 397, 459, 532, 600,
         676, 755,
         832, 896, 1002, 1055},
        {208, 194, 207, 223, 242, 265, 296, 335, 384, 445, 511, 583,
         656, 726,
         817, 889, 988, 1044},
        {262, 254, 220, 239, 261, 283, 310, 344, 388, 442, 496, 568,
         635, 725,
         791, 890, 970, 1036},
        {311, 306, 299, 251, 280, 304, 330, 361, 396, 443, 500, 563,
         630, 704,
         796, 880, 951, 1033},
        {358, 359, 350, 338, 293, 323, 352, 381, 416, 455, 511, 566,
         635, 706,
         780, 859, 946, 1039},
        {411, 409, 405, 392, 370, 340, 375, 406, 439, 478, 525, 580,
         644, 714,
         790, 873, 961, 1050},
        {462, 463, 459, 450, 443, 400, 394, 432, 467, 501, 546, 595,
         650, 711,
         786, 873, 963, 1050},
        {522, 525, 529, 524, 516, 518, 438, 454, 495, 532, 565, 621,
         671, 741,
         813, 874, 951, 1034},
        {589, 593, 590, 583, 579, 568, 565, 483, 517, 560, 600, 644,
         691, 752,
         821, 904, 978, 1048},
        {658, 655, 666, 657, 649, 653, 649, 587, 549, 592, 674, 674,
         728, 782,
         833, 902, 992, 1084},
        {738, 742, 747, 725, 721, 729, 699, 730, 626, 622, 668, 721,
         765, 809,
         887, 938, 1001, 1109},
        {838, 838, 810, 809, 790, 800, 769, 815, 757, 679, 704, 755,
         806, 854,
         901, 974, 1034, 1105},
        {942, 946, 925, 901, 918, 895, 919, 897, 933, 890, 785, 797,
         859, 908,
         976, 1020, 1115, 1173},
        {1059, 1061, 1056, 1061, 1074, 1031, 1036, 1036, 993, 1038, 932,
         852,
         878,
         943, 1003, 1074, 1131, 1200},
        {1069, 1076, 1083, 1095, 1102, 1091, 1126, 1156, 1103, 1149,
         1157,
         1036,
         972, 1007, 1064, 1124, 1209, 1283},
        {1338, 1350, 1356, 1354, 1324, 1301, 1312, 1318, 1257, 1239,
         1297,
         1233,
         1089, 1059, 1106, 1180, 1218, 1317},
        {1409, 1398, 1367, 1336, 1313, 1313, 1409, 1354, 1317, 1287,
         1353,
         1386,
         1279, 1158, 1141, 1188, 1260, 1335},
        {1328, 1332, 1342, 1369, 1405, 1451, 1502, 1524, 1506, 1477,
         1522,
         1594,
         1572, 1436, 1328, 1325, 1382, 1446}
    };
    static float alp[21][18] = {
        {.268, .269, .335, .377, .327, .286, .273, .270, .271, .268,
         .267,
         .264,
         .264, .264, .261, .256, .248, .245},
        {.261, .256, .254, .282, .327, .355, .321, .293, .287, .271,
         .267,
         .272,
         .270, .270, .268, .268, .264, .263},
        {.266, .264, .257, .252, .267, .289, .325, .339, .319, .301,
         .292,
         .284,
         .281, .281, .277, .282, .276, .274},
        {.262, .274, .258, .251, .247, .254, .273, .291, .316, .322,
         .320,
         .302,
         .294, .290, .287, .292, .283, .277},
        {.322, .275, .264, .259, .250, .245, .273, .255, .271, .284,
         .294,
         .308,
         .296, .299, .288, .289, .282, .278},
        {.267, .300, .260, .268, .245, .242, .243, .242, .239, .246,
         .267,
         .277,
         .280, .290, .282, .281, .274, .271},
        {.259, .274, .275, .252, .265, .248, .249, .237, .238, .236,
         .247,
         .254,
         .254, .271, .268, .267, .258, .262},
        {.260, .255, .268, .268, .268, .264, .248, .239, .229, .240,
         .236,
         .234,
         .238, .244, .252, .251, .244, .255},
        {.255, .255, .244, .247, .317, .246, .255, .244, .237, .231,
         .227,
         .231,
         .235, .232, .235, .241, .237, .245},
        {.256, .254, .254, .249, .227, .319, .253, .253, .240, .237,
         .238,
         .233,
         .231, .230, .228, .234, .227, .241},
        {.257, .254, .252, .235, .253, .240, .284, .251, .246, .241,
         .235,
         .228,
         .222, .225, .225, .219, .228, .233},
        {.244, .240, .245, .238, .248, .230, .283, .252, .244, .244,
         .238,
         .235,
         .234, .236, .228, .224, .225, .231},
        {.244, .241, .244, .237, .237, .249, .219, .324, .239, .245,
         .242,
         .242,
         .232, .233, .221, .227, .231, .218},
        {.241, .245, .249, .239, .243, .250, .217, .254, .308, .237,
         .247,
         .244,
         .234, .228, .233, .224, .227, .226},
        {.243, .243, .232, .227, .235, .253, .227, .220, .320, .270,
         .243,
         .252,
         .248, .238, .234, .241, .225, .227},
        {.225, .226, .234, .230, .226, .233, .249, .225, .216, .300,
         .286,
         .237,
         .240, .247, .243, .234, .231, .238},
        {.268, .260, .247, .238, .233, .241, .254, .248, .207, .227,
         .315,
         .260,
         .226, .237, .240, .239, .239, .240},
        {.248, .246, .238, .226, .213, .221, .226, .226, .204, .194,
         .248,
         .316,
         .234, .216, .236, .233, .221, .230},
        {.200, .202, .198, .194, .206, .207, .227, .224, .207, .185,
         .198,
         .275,
         .315, .233, .229, .231, .233, .236},
        {.202, .209, .221, .226, .230, .245, .202, .257, .246, .225,
         .215,
         .246,
         .320, .321, .244, .239, .251, .253},
        {.246, .248, .255, .265, .274, .285, .292, .284, .273, .250,
         .225,
         .239,
         .295, .352, .320, .258, .260, .269}
    };
    static float ns[21] = { 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1, 2.2,
        2.3,
        2.4, 2.5, 2.6, 2.7,
        2.8, 2.9, 3.0
    };
    static float np[18] = { 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1, 2.2, 2.3, 2.4, 2.5,
        2.6,
        2.7,
        2.8, 2.9, 3.0
    };

    float NP, NS, sigma, alpha, sig1, sig2, sig3, sig4, alp1, alp2, alp3, alp4, siga, sigb;
    float sigq, alpa, alpb, alpq;
    int i, j, k, l;
    double pi = 3.1415926536;
    double wN, lnwN;
    double nfac;

    if (ll == 0)
        NS = neffl;
    if (lh == 0)
        NS = neffh;
    if (ll == 1)
        NP = neffl;
    if (lh == 1)
        NP = neffh;

    if (NS <= 1.0 || NS > 3.0 || NP <= 1.3 || NP > 3.0)
        return (0);
    if (charge > 1.0)
        return (2);

    for (i = 0; i < 21; i++) {
        if (NS <= ns[i + 1]) {
            k = i;
            break;
        }
    }

    for (i = 0; i < 18; i++) {
        if (NP <= np[i + 1]) {
            l = i;
            break;
        }
    }

    sig1 = sig[k][l];
    sig2 = sig[k + 1][l];
    sig3 = sig[k][l + 1];
    sig4 = sig[k + 1][l + 1];

    siga = sig1 + (sig3 - sig1) * (NP - np[l]) / (np[l + 1] - np[l]);
    sigb = sig2 + (sig4 - sig2) * (NP - np[l]) / (np[l + 1] - np[l]);
    *s = siga + (sigb - siga) * (NS - ns[k]) / (ns[k + 1] - ns[k]);

    alp1 = alp[k][l];
    alp2 = alp[k + 1][l];
    alp3 = alp[k][l + 1];
    alp4 = alp[k + 1][l + 1];

    alpa = alp1 + (alp3 - alp1) * (NP - np[l]) / (np[l + 1] - np[l]);
    alpb = alp2 + (alp4 - alp2) * (NP - np[l]) / (np[l + 1] - np[l]);
    *a = alpa + (alpb - alpa) * (NS - ns[k]) / (ns[k + 1] - ns[k]);

    return (1);
}

int pd(neffl, neffh, ll, lh, s, a, charge)
double neffl, neffh, charge;
double *s, *a;
int ll, lh;
{
    static float sig[18][18] = {
        {425, 461, 507, 566, 630, 706, 799, 889, 995, 1083, 1191, 1334,
         1478,
         1608, 1790, 1870, 1936, 2140},
        {429, 460, 505, 565, 633, 704, 795, 896, 985, 1082, 1199, 1340,
         1487,
         1611, 1795, 1872, 1937, 2136},
        {419, 451, 501, 556, 627, 700, 785, 891, 977, 1088, 1212, 1346,
         1493,
         1604, 1793, 1863, 1930, 2144},
        {402, 437, 489, 544, 614, 695, 779, 875, 975, 1102, 1221, 1350,
         1488,
         1591, 1774, 1844, 1919, 2126},
        {384, 418, 467, 529, 595, 674, 769, 856, 976, 1108, 1224, 1338,
         1467,
         1570, 1743, 1817, 1900, 2118},
        {366, 397, 443, 505, 576, 651, 755, 841, 973, 1095, 1210, 1308,
         1435,
         1545, 1702, 1786, 1878, 2081},
        {356, 387, 432, 489, 562, 635, 722, 841, 961, 1078, 1175, 1273,
         1397,
         1517, 1672, 1763, 1863, 2034},
        {359, 388, 431, 479, 545, 624, 707, 834, 943, 1059, 1158, 1256,
         1368,
         1490, 1647, 1747, 1849, 1998},
        {361, 394, 436, 483, 547, 615, 704, 817, 920, 1027, 1124, 1238,
         1358,
         1465, 1624, 1736, 1838, 1978},
        {400, 382, 440, 489, 546, 610, 690, 817, 897, 998, 1115, 1201,
         1351,
         1453,
         1599, 1728, 1829, 1953},
        {474, 461, 416, 491, 549, 612, 701, 806, 883, 974, 1078, 1194,
         1310,
         1456,
         1569, 1716, 1818, 1925},
        {531, 518, 507, 463, 547, 615, 694, 784, 881, 958, 1047, 1153,
         1297,
         1432,
         1547, 1688, 1809, 1901},
        {594, 585, 577, 564, 513, 615, 695, 779, 879, 949, 1041, 1145,
         1264,
         1388,
         1544, 1644, 1804, 1879},
        {675, 659, 651, 639, 632, 576, 695, 782, 879, 957, 1046, 1141,
         1254,
         1391,
         1524, 1614, 1793, 1871},
        {739, 734, 726, 719, 715, 708, 663, 776, 901, 971, 1022, 1117,
         1232,
         1355,
         1478, 1616, 1766, 1887},
        {819, 821, 805, 784, 773, 761, 736, 761, 888, 958, 1044, 1145,
         1237,
         1346,
         1487, 1614, 1721, 1891},
        {899, 895, 871, 852, 856, 861, 854, 759, 883, 984, 1027, 1113,
         1226,
         1355,
         1467, 1568, 1703, 1885},
        {973, 946, 955, 925, 939, 927, 902, 920, 870, 987, 1061, 1145,
         1234,
         1319,
         1439, 1552, 1722, 1859}
    };
    static float alp[18][18] = {
        {.281, .288, .283, .282, .278, .281, .272, .274, .268, .257,
         .251,
         .243,
         .246, .251, .254, .268, .304, .308},
        {.290, .297, .291, .290, .286, .282, .277, .275, .267, .254,
         .252,
         .244,
         .250, .257, .260, .274, .308, .312},
        {.294, .299, .293, .294, .288, .289, .281, .276, .265, .256,
         .251,
         .247,
         .258, .264, .268, .283, .318, .317},
        {.297, .298, .302, .300, .289, .295, .290, .276, .264, .256,
         .260,
         .258,
         .268, .277, .281, .292, .330, .327},
        {.305, .311, .313, .315, .305, .304, .299, .279, .271, .272,
         .273,
         .276,
         .285, .290, .293, .302, .340, .340},
        {.292, .294, .303, .305, .301, .307, .290, .277, .274, .278,
         .287,
         .288,
         .295, .302, .306, .312, .343, .346},
        {.268, .277, .279, .285, .285, .290, .279, .278, .280, .283,
         .295,
         .296,
         .305, .310, .313, .315, .342, .346},
        {.288, .285, .280, .278, .278, .277, .272, .271, .279, .288,
         .297,
         .305,
         .310, .313, .311, .310, .335, .338},
        {.314, .304, .292, .282, .275, .275, .262, .272, .290, .293,
         .299,
         .307,
         .308, .310, .303, .302, .325, .328},
        {.346, .329, .313, .295, .283, .275, .264, .274, .288, .302,
         .307,
         .310,
         .306, .307, .292, .296, .315, .320},
        {.320, .295, .326, .318, .294, .277, .275, .271, .293, .303,
         .305,
         .309,
         .309, .303, .294, .294, .310, .313},
        {.304, .310, .297, .320, .317, .297, .283, .274, .298, .305,
         .308,
         .311,
         .313, .300, .290, .293, .305, .306},
        {.314, .313, .308, .297, .325, .314, .293, .276, .292, .309,
         .314,
         .308,
         .303, .296, .286, .291, .301, .302},
        {.308, .311, .307, .312, .288, .340, .305, .285, .294, .310,
         .315,
         .309,
         .296, .285, .281, .288, .298, .295},
        {.313, .310, .315, .303, .313, .294, .331, .286, .294, .307,
         .320,
         .316,
         .303, .281, .278, .285, .290, .292},
        {.315, .306, .308, .297, .295, .283, .334, .297, .280, .294,
         .314,
         .321,
         .313, .291, .280, .279, .287, .290},
        {.308, .304, .305, .297, .279, .285, .251, .278, .278, .284,
         .297,
         .314,
         .307, .289, .274, .274, .274, .291},
        {.301, .299, .298, .285, .265, .279, .241, .285, .260, .286,
         .302,
         .306,
         .302, .288, .277, .263, .271, .293}
    };
    static float np[18] = { 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1, 2.2, 2.3, 2.4, 2.5,
        2.6,
        2.7, 2.8, 2.9, 3.0
    };
    static float nd[18] = { 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0, 3.1, 3.2, 3.3, 3.4, 3.5,
        3.6,
        3.7, 3.8, 3.9, 4.0
    };

    float NP, ND, sigma, alpha, sig1, sig2, sig3, sig4, alp1, alp2, alp3, alp4, siga, sigb;
    float sigq, alpa, alpb, alpq;
    int i, j, k, l;
    double pi = 3.1415926536;
    double wN, lnwN;
    double nfac;

    if (ll == 1)
        NP = neffl;
    if (lh == 1)
        NP = neffh;
    if (ll == 2)
        ND = neffl;
    if (lh == 2)
        ND = neffh;

    if (NP <= 1.3 || NP > 3.0 || ND <= 2.3 || ND > 4.0)
        return (0);
    if (charge > 1.0)
        return (2);

    for (i = 0; i < 18; i++) {
        if (NP <= np[i + 1]) {
            k = i;
            break;
        }
    }

    for (i = 0; i < 18; i++) {
        if (ND <= nd[i + 1]) {
            l = i;
            break;
        }
    }

    sig1 = sig[k][l];
    sig2 = sig[k + 1][l];
    sig3 = sig[k][l + 1];
    sig4 = sig[k + 1][l + 1];

    siga = sig1 + (sig3 - sig1) * (ND - nd[l]) / (nd[l + 1] - nd[l]);
    sigb = sig2 + (sig4 - sig2) * (ND - nd[l]) / (nd[l + 1] - nd[l]);
    *s = siga + (sigb - siga) * (NP - np[k]) / (np[k + 1] - np[k]);

    alp1 = alp[k][l];
    alp2 = alp[k + 1][l];
    alp3 = alp[k][l + 1];
    alp4 = alp[k + 1][l + 1];

    alpa = alp1 + (alp3 - alp1) * (ND - nd[l]) / (nd[l + 1] - nd[l]);
    alpb = alp2 + (alp4 - alp2) * (ND - nd[l]) / (nd[l + 1] - nd[l]);
    *a = alpa + (alpb - alpa) * (NP - np[k]) / (np[k + 1] - np[k]);

    return (1);
}

int df(neffl, neffh, ll, lh, s, a, charge)
double neffl, neffh, charge;
double *s, *a;
int ll, lh;
{
    static float sig[18][18] = {
        {808, 873, 958, 1059, 1175, 1306, 1453, 1615, 1793, 1979, 2121,
         2203,
         2461, 2604, 2764, 2757, 2784, 3156},
        {798, 866, 953, 1052, 1172, 1299, 1450, 1606, 1776, 1967, 2114,
         2196,
         2451, 2601, 2763, 2767, 2783, 3142},
        {781, 848, 934, 1030, 1149, 1276, 1416, 1596, 1751, 1944, 2100,
         2188,
         2436, 2594, 2767, 2777, 2795, 3123},
        {766, 831, 915, 1010, 1124, 1239, 1398, 1564, 1729, 1912, 2083,
         2180,
         2426, 2585, 2776, 2790, 2808, 3106},
        {750, 814, 897, 987, 1097, 1201, 1355, 1530, 1718, 1875, 2060,
         2171,
         2414,
         2575, 2779, 2809, 2820, 3103},
        {733, 797, 872, 950, 1049, 1166, 1326, 1502, 1670, 1851, 2026,
         2165,
         2396,
         2562, 2779, 2827, 2832, 3099},
        {726, 786, 853, 936, 1011, 1128, 1303, 1472, 1649, 1844, 1979,
         2159,
         2371,
         2548, 2778, 2840, 2848, 3103},
        {709, 783, 847, 912, 1002, 1093, 1270, 1419, 1606, 1787, 1951,
         2139,
         2335,
         2533, 2775, 2847, 2863, 3104},
        {758, 721, 838, 907, 1010, 1066, 1211, 1401, 1600, 1774, 1972,
         2098,
         2313,
         2528, 2781, 2857, 2892, 3121},
        {869, 882, 820, 870, 1003, 1098, 1165, 1368, 1527, 1735, 1896,
         2030,
         2288,
         2534, 2776, 2844, 2902, 3123},
        {970, 967, 934, 938, 918, 1130, 1194, 1287, 1507, 1679, 1821,
         2021,
         2271,
         2525, 2732, 2786, 2882, 3085},
        {1079, 1043, 1056, 1007, 1014, 1021, 1200, 1326, 1424, 1668,
         1818,
         1988,
         2242, 2493, 2672, 2719, 2853, 3035},
        {1174, 1173, 1127, 1154, 1104, 1099, 1169, 1288, 1442, 1580,
         1704,
         1882,
         2136, 2400, 2561, 2648, 2832, 2994},
        {1285, 1278, 1269, 1225, 1252, 1229, 1116, 1343, 1380, 1594,
         1710,
         1874,
         2054, 2309, 2484, 2607, 2813, 2932},
        {1440, 1408, 1422, 1380, 1383, 1341, 1361, 1192, 1448, 1454,
         1675,
         1873,
         2069, 2246, 2432, 2610, 2811, 2878},
        {1572, 1545, 1553, 1517, 1481, 1502, 1469, 1349, 1373, 1561,
         1586,
         1781,
         2072, 2301, 2490, 2626, 2754, 2832},
        {1698, 1701, 1694, 1641, 1617, 1651, 1566, 1600, 1374, 1547,
         1698,
         1749,
         1989, 2289, 2511, 2594, 2689, 2774},
        {1870, 1841, 1786, 1752, 1777, 1757, 1666, 1732, 1522, 1533,
         1707,
         1817,
         1928, 2194, 2435, 2574, 2665, 2742}
    };
    static float alp[18][18] = {
        {.295, .286, .299, .300, .307, .310, .311, .311, .316, .319,
         .325,
         .351,
         .364, .369, .372, .379, .373, .351},
        {.295, .295, .301, .302, .311, .316, .314, .314, .320, .321,
         .324,
         .349,
         .361, .365, .368, .374, .368, .349},
        {.286, .298, .302, .304, .311, .323, .321, .319, .324, .323,
         .323,
         .345,
         .355, .358, .362, .367, .361, .343},
        {.290, .295, .307, .316, .322, .329, .326, .325, .329, .324,
         .321,
         .343,
         .350, .351, .354, .360, .358, .337},
        {.292, .299, .307, .321, .327, .336, .333, .330, .330, .320,
         .321,
         .338,
         .344, .344, .345, .352, .352, .332},
        {.291, .299, .309, .323, .335, .339, .335, .333, .327, .323,
         .319,
         .333,
         .336, .336, .336, .344, .345, .329},
        {.297, .302, .312, .321, .340, .338, .333, .327, .325, .319,
         .318,
         .324,
         .329, .330, .330, .336, .337, .325},
        {.319, .314, .317, .327, .334, .344, .339, .327, .323, .318,
         .312,
         .318,
         .319, .322, .322, .326, .327, .316},
        {.333, .328, .339, .325, .359, .351, .332, .325, .322, .311,
         .309,
         .310,
         .311, .316, .314, .317, .321, .313},
        {.274, .273, .323, .412, .318, .339, .359, .328, .324, .311,
         .309,
         .325,
         .322, .315, .318, .319, .325, .314},
        {.297, .296, .273, .302, .436, .325, .354, .335, .326, .311,
         .314,
         .330,
         .323, .324, .325, .323, .330, .314},
        {.284, .295, .296, .280, .300, .438, .322, .348, .332, .318,
         .320,
         .332,
         .335, .334, .335, .331, .333, .309},
        {.280, .278, .285, .297, .279, .320, .445, .319, .320, .324,
         .328,
         .338,
         .348, .346, .345, .336, .328, .300},
        {.280, .273, .267, .273, .284, .268, .343, .390, .323, .308,
         .318,
         .325,
         .343, .348, .346, .337, .311, .286},
        {.277, .270, .260, .266, .276, .263, .294, .408, .337, .324,
         .299,
         .308,
         .331, .334, .345, .327, .315, .280},
        {.270, .262, .258, .260, .273, .273, .262, .375, .410, .298,
         .312,
         .294,
         .313, .331, .328, .322, .307, .270},
        {.271, .267, .262, .264, .274, .269, .261, .323, .351, .359,
         .294,
         .325,
         .310, .318, .321, .315, .291, .268},
        {.275, .276, .272, .276, .279, .270, .264, .295, .393, .340,
         .319,
         .287,
         .320, .330, .316, .302, .280, .261}
    };
    static float nd[18] = { 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0, 3.1, 3.2, 3.3, 3.4, 3.5,
        3.6,
        3.7, 3.8, 3.9, 4.0
    };
    static float nf[18] = { 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0, 4.1, 4.2, 4.3, 4.4, 4.5,
        4.6,
        4.7, 4.8, 4.9, 5.0
    };

    float ND, NF, sigma, alpha, sig1, sig2, sig3, sig4, alp1, alp2, alp3, alp4, siga, sigb;
    float sigq, alpa, alpb, alpq;
    int i, j, k, l;
    double pi = 3.1415926536;
    double wN, lnwN;
    double nfac;

    if (ll == 2)
        ND = neffl;
    if (lh == 2)
        ND = neffh;
    if (ll == 3)
        NF = neffl;
    if (lh == 3)
        NF = neffh;

    if (ND <= 2.3 || ND > 4.0 || NF <= 3.3 || NF > 5.0)
        return (0);
    if (charge > 1.0)
        return (2);

    for (i = 0; i < 18; i++) {
        if (ND <= nd[i + 1]) {
            k = i;
            break;
        }
    }

    for (i = 0; i < 18; i++) {
        if (NF <= nf[i + 1]) {
            l = i;
            break;
        }
    }

    sig1 = sig[k][l];
    sig2 = sig[k + 1][l];
    sig3 = sig[k][l + 1];
    sig4 = sig[k + 1][l + 1];

    siga = sig1 + (sig3 - sig1) * (NF - nf[l]) / (nf[l + 1] - nf[l]);
    sigb = sig2 + (sig4 - sig2) * (NF - nf[l]) / (nf[l + 1] - nf[l]);
    *s = siga + (sigb - siga) * (ND - nd[k]) / (nd[k + 1] - nd[k]);

    alp1 = alp[k][l];
    alp2 = alp[k + 1][l];
    alp3 = alp[k][l + 1];
    alp4 = alp[k + 1][l + 1];

    alpa = alp1 + (alp3 - alp1) * (NF - nf[l]) / (nf[l + 1] - nf[l]);
    alpb = alp2 + (alp4 - alp2) * (NF - nf[l]) / (nf[l + 1] - nf[l]);
    *a = alpa + (alpb - alpa) * (ND - nd[k]) / (nd[k + 1] - nd[k]);

    return (1);
}

double gammln(xx)
double xx;
{
    double x, y, tmp, ser;
    static double cof[6] = { 76.18009172947146, -86.50532032941677,
        24.01409824083091, -1.231739572450155, 0.1208650973866179e-02,
        -0.5395239384953e-05
    };
    int j;

    y = x = xx;
    tmp = x + 5.5;
    tmp -= (x + 0.5) * log(tmp);
    ser = 1.000000000190015;
    for (j = 0; j <= 5; j++)
        ser += cof[j] / ++y;
    return -tmp + log(2.5066282746310005 * ser / x);
}
