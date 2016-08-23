#ifndef _NAMES_H
#define _NAMES_H

extern const char *names_vendor(u_int16_t vendorid);
extern const char *names_product(u_int16_t vendorid, u_int16_t productid);
extern const char *names_class(u_int8_t classid);
extern const char *names_subclass(u_int8_t classid, u_int8_t subclassid);
extern const char *names_protocol(u_int8_t classid, u_int8_t subclassid, u_int8_t protocolid);
extern int get_vendor_string(char *buf, size_t size, u_int16_t vid);
extern int get_product_string(char *buf, size_t size, u_int16_t vid, u_int16_t pid);
extern int get_class_string(char *buf, size_t size, u_int8_t cls);
extern int get_subclass_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls);
extern int get_protocol_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls, u_int8_t proto);

extern int names_init(void);
extern void names_exit(void);

/* ---------------------------------------------------------------------- */
#endif /* _NAMES_H */
