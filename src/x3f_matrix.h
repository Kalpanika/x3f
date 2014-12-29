extern void x3f_3x3_1x3_mul(float *a, float *b, float *c);
extern void x3f_3x3_3x3_mul(float *a, float *b, float *c);

extern void x3f_3x3_diag(float *a, float *b);

extern void x3f_1x3_print(float *a);
extern void x3f_3x3_print(float *a);

extern void x3f_XYZ_to_ProPhotoRGB(float *a);
extern void x3f_XYZ_to_AdobeRGB(float *a);
extern void x3f_XYZ_to_AdobeRGB_D50(float *a);
extern void x3f_XYZ_to_sRGB(float *a);
