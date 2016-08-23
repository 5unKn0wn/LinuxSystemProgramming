#include <libudev.h>
#include <libusb.h>
#include <string.h>
#include <stdio.h>

#include "names.h"

#define le16_to_cpu(x) libusb_cpu_to_le16(libusb_cpu_to_le16(x))

int printdev(libusb_device *dev);
int main(void) {
	libusb_device **devs;
	libusb_context *ctx = NULL;
	int r;
	ssize_t cnt;

	names_init();

	r = libusb_init(&ctx) < 0;
	if (r < 0) {
		printf("Error : %d\n", r);
		return -1;
	}	

	cnt = libusb_get_device_list(ctx, &devs);
	if (cnt < 0) {
		printf("Error : Getting Device list failed.\n");
		return -1;
	}

	for (int i = 0; i < cnt; i++)
		printdev(devs[i]);

	names_exit();
	libusb_free_device_list(devs, 1);
	libusb_exit(ctx);

	return 0;
}

int printdev(libusb_device *dev) {
	static const char * const encryption_type[] = { "UNSECURE", "WIRED", "CCM_1", "RSA_1", "RESERVED" };
	static const char * const typeattr[] = { "Control", "Isochronous", "Bulk", "Interrupt" };
	static const char * const syncattr[] = { "None", "Asynchronous", "Adaptive", "Synchronous" };
	static const char * const usage[] = { "Data", "Feedback", "Implicit feedback Data", "(reserved)" };
	static const char * const hb[] = { "1x", "2x", "3x", "(?\?)" };
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	struct libusb_interface *interface;
	struct libusb_interface_descriptor *altsetting;
	struct libusb_endpoint_descriptor *endpoint;
	unsigned char *buf;
	char vendor[128], product[128];
	char cls[128], subcls[128], proto[128];
	char *mfg, *prod, *serial;
	char *cfg;
	char *ifstr;
	unsigned int wmax;
	int r, bnum, dnum;
	int b_encryption_type;
	int size;

	r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		printf("Error : Device Descriptor getting failed.\n");
		return -1;
	}

	bnum = libusb_get_bus_number(dev);
	dnum = libusb_get_device_address(dev);

	get_vendor_string(vendor, sizeof(vendor), desc.idVendor);
	get_product_string(product, sizeof(product), desc.idVendor, desc.idProduct);
	get_class_string(cls, sizeof(cls), desc.bDeviceClass);
	get_subclass_string(subcls, sizeof(subcls),	desc.bDeviceClass, desc.bDeviceSubClass);
	get_protocol_string(proto, sizeof(proto), desc.bDeviceClass, desc.bDeviceSubClass, desc.bDeviceProtocol);

	printf("Bus %03u Device %03u: ID %04x:%04x %s %s\n", bnum, dnum, desc.idVendor, desc.idProduct, vendor, product);

	printf("Device Descriptor:\n");
	printf("  bLength             %5u\n", desc.bLength);
	printf("  bDescriptorType     %5u\n", desc.bDescriptorType);
	printf("  bcdUSB              %2x.%02x\n", desc.bcdUSB >> 8, desc.bcdUSB & 0xff);
	printf("  bDeviceClass        %5u %s\n", desc.bDeviceClass, cls);
	printf("  bDeviceSubClass     %5u %s\n", desc.bDeviceSubClass, subcls);
	printf("  bDeviceProtocol     %5u %s\n", desc.bDeviceProtocol, proto);
	printf("  bMaxPacketSize0     %5u\n", desc.bMaxPacketSize0);
	printf("  idVendor           0x%04x %s\n", desc.idVendor, vendor);
	printf("  idProduct          0x%04x %s\n", desc.idProduct, product);
	printf("  bcdDevice           %2x.%02x\n", desc.bcdDevice >> 8, desc.bcdDevice & 0xff);
	printf("  iManufacturer       %5u\n", desc.iManufacturer);
	printf("  iProduct            %5u\n", desc.iProduct);
	printf("  iSerial             %5u\n", desc.iSerialNumber);
	printf("  bNumConfigurations  %5u\n", desc.bNumConfigurations);

	r = libusb_get_config_descriptor(dev, 0, &config);
	if (r < 0) {
		printf("Error : Device Config Descriptor getting failed.\n");
		return -1;
	}

	
	printf("  Configuration Descriptor:\n");
	printf("    bLength             %5u\n", config->bLength);
	printf("    bDescriptorType     %5u\n", config->bDescriptorType);
	printf("    wTotalLength        %5u\n", le16_to_cpu(config->wTotalLength));
	printf("    bNumInterfaces      %5u\n", config->bNumInterfaces);
	printf("    bConfigurationValue %5u\n", config->bConfigurationValue);
	printf("    iConfiguration      %5u\n", config->iConfiguration);
	printf("    bmAttributes         0x%02x\n", config->bmAttributes);	
	if (!(config->bmAttributes & 0x80))
		printf("      (Missing must-be-set bit!)\n");
	if (config->bmAttributes & 0x40)
		printf("      Self Powered\n");
	else
		printf("      (Bus Powered)\n");
	if (config->bmAttributes & 0x20)
		printf("      Remote Wakeup\n");
	if (config->bmAttributes & 0x10)
		printf("      Battery Powered\n");
	printf("    MaxPower            %5umA\n", config->MaxPower * (desc.bcdUSB >= 0x0300 ? 8 : 2));

	for (int i = 0; i < config->bNumInterfaces; i++) {
		interface = &config->interface[i];
		for (int j = 0; j < interface->num_altsetting; j++) {
			altsetting = &interface->altsetting[j];

			get_class_string(cls, sizeof(cls), altsetting->bInterfaceClass);
			get_subclass_string(subcls, sizeof(subcls), altsetting->bInterfaceClass, altsetting->bInterfaceSubClass);
			get_protocol_string(proto, sizeof(proto), altsetting->bInterfaceClass, altsetting->bInterfaceSubClass, altsetting->bInterfaceProtocol);

			printf("    Interface Descriptor:\n");
			printf("      bLength             %5u\n", altsetting->bLength);
			printf("      bDescriptorType     %5u\n", altsetting->bDescriptorType);
			printf("      bInterfaceNumber    %5u\n", altsetting->bInterfaceNumber);
			printf("      bAlternateSetting   %5u\n", altsetting->bAlternateSetting);
			printf("      bNumEndpoints       %5u\n", altsetting->bNumEndpoints);
			printf("      bInterfaceClass     %5u %s\n", altsetting->bInterfaceClass, cls);
			printf("      bInterfaceSubClass  %5u %s\n", altsetting->bInterfaceSubClass, subcls);
			printf("      bInterfaceProtocol  %5u %s\n", altsetting->bInterfaceProtocol, proto);
			printf("      iInterface          %5u\n", altsetting->iInterface);

			for (int k = 0; k < altsetting->bNumEndpoints; k++) {
				endpoint = &altsetting->endpoint[k];

				wmax = le16_to_cpu(endpoint->wMaxPacketSize);

				printf("      Endpoint Descriptor:\n");
				printf("        bLength             %5u\n", endpoint->bLength);
				printf("        bDescriptorType     %5u\n", endpoint->bDescriptorType);
				printf("        bEndpointAddress     0x%02x  EP %u %s\n", endpoint->bEndpointAddress, endpoint->bEndpointAddress & 0xf, (endpoint->bEndpointAddress & 0x80) ? "IN" : "OUT");
				printf("        bmAttributes        %5u\n", endpoint->bmAttributes);
				printf("          Transfer Type            %s\n", typeattr[endpoint->bmAttributes & 3]);
				printf("          Synch Type               %s\n", syncattr[(endpoint->bmAttributes >> 2) & 3]);
				printf("          Usage Type               %s\n", usage[(endpoint->bmAttributes >> 4) & 3]);
				printf("        wMaxPacketSize     0x%04x  %s %d bytes\n", wmax, hb[(wmax >> 11) & 3], wmax & 0x7ff);
				printf("        bInterval           %5u\n",  endpoint->bInterval);

				if (endpoint->bLength == 9) {
					printf("        bRefresh            %5u\n", endpoint->bRefresh);
					printf("        bSynchAddress       %5u\n", endpoint->bSynchAddress);
				}
			}
		}
	}

	printf("\n");

	return 0;
}
