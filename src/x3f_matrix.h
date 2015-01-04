extern void x3f_scalar_3x1_mul(double a, double *b, double *c);
extern void x3f_scalar_3x3_mul(double a, double *b, double *c);

extern void x3f_3x3_3x1_mul(double *a, double *b, double *c);
extern void x3f_3x3_3x3_mul(double *a, double *b, double *c);

extern void x3f_3x3_diag(double *a, double *b);

extern void x3f_3x1_print(double *a);
extern void x3f_3x3_print(double *a);

extern void x3f_XYZ_to_ProPhotoRGB(double *a);

extern void x3f_XYZ_to_AdobeRGB(double *a);
extern void x3f_AdobeRGB_to_XYZ(double *a);

extern void x3f_XYZ_to_sRGB(double *a);

extern void x3f_CIERGB_to_XYZ(double *a);

extern void x3f_Bradford_D50_to_D65(double *a);
