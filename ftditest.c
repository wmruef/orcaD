#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>
#include <ftdi.h>

enum opModeTypes { READ_OP, LIST_OP };

static struct termios stored_settings;


void reset_keypress(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}



int main(int argc, char**argv)
{
  int i, ret;
  int vid = 0x0;
  int pid = 0x0;
  int baudrate = 9600;
  int interface = INTERFACE_ANY;
  char *sid = NULL;
  char buf[1024];
  struct ftdi_context ftdic;
  struct ftdi_device_list *devlist, *curdev, *tmpdev;
  //char key;
  char manufacturer[128], description[128], serial[128];
  int opMode = LIST_OP;
  struct usb_bus *bus;
  struct usb_device *dev;


  while ((i = getopt(argc, argv, "rli:v:p:s:b:")) != -1)
  {
    switch (i)
    {
      case 'r': // Read operation
                opMode = READ_OP;
                break;
      case 'l': // Read operation
                opMode = LIST_OP;
                break;
      case 'i': // 0=ANY, 1=A, 2=B, 3=C, 4=D
                interface = strtoul(optarg, NULL, 0);
                break;
      case 'v':
                //vid = strtoul(optarg, NULL, 0);
                sscanf(optarg, "%x", &vid);
                break;
      case 'p':
                //pid = strtoul(optarg, NULL, 0);
                sscanf(optarg, "%x", &pid);
                break;
      case 's':
                sid = (char *)malloc(sizeof(char) * 128);
                if ( sid )
                  strncpy( sid, optarg, 128 );
                break;
      case 'b':
                baudrate = strtoul(optarg, NULL, 0);
                break;
      default:
                fprintf(stderr, "usage: %s -r|l [-i interface] [-v vender_id] "
                        "[-p product_id] [-s serial_id] [-b baudrate]\n", 
                        *argv);
                exit(-1);
    }
  }

/*
    if ((ret = ftdi_usb_open(&ftdic, 0x0403, 0x6001)) < 0)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
        return EXIT_FAILURE;
    }

    // Read out FTDIChip-ID of R type chips
    if (ftdic.type == TYPE_R)
    {
        unsigned int chipid;
        printf("ftdi_read_chipid: %d\n", ftdi_read_chipid(&ftdic, &chipid));
        printf("FTDI chipid: %X\n", chipid);
    }
*/

  if ( opMode == LIST_OP )
  {
    usb_init();
    usb_find_busses();
    usb_find_devices();
  
    devlist = NULL;
    curdev = NULL;
    ret = 0;
    for (bus = usb_get_busses(); bus; bus = bus->next)
    {
      for (dev = bus->devices; dev; dev = dev->next)
      {
        tmpdev = (struct ftdi_device_list*)
                    malloc(sizeof(struct ftdi_device_list));
        if (!tmpdev)
        {
          printf("Could not allocate memory for ftdi_device_list!\n");
          return( EXIT_FAILURE );
        }
        // Initialize struct
        (tmpdev)->next = NULL;
        (tmpdev)->dev = dev;
  
        if ( devlist == NULL )
          devlist = tmpdev;
  
        if ( curdev )
          curdev->next = tmpdev;
  
        curdev = tmpdev;
        ret++;
      }
    }
  
    printf("Number of USB devices found: %d\n", ret);
  
    i = 0;
    for (curdev = devlist; curdev != NULL; i++)
    {
      printf("Checking device: %d\n", i);
      if ((ret = ftdi_usb_get_strings(&ftdic, curdev->dev, manufacturer, 
                                  128, description, 128, serial, 128 )) < 0 )
      {
        fprintf(stderr, "ftdi_usb_get_strings failed: %d (%s)\n", ret, 
                        ftdi_get_error_string(&ftdic));
        return EXIT_FAILURE;
      }
      printf(" - Manufacturer: %s, Description: %si, Serial: %s\n", 
             manufacturer, description, serial);
      curdev = curdev->next;
    }

    ftdi_list_free(&devlist);

  }else if ( opMode == READ_OP )
  {
    if ( vid == 0 && pid == 0 )
    {
          fprintf(stderr, "usage: %s -r|l [-i interface] [-v vender_id] "
                  "[-p product_id] [-s serial_id] [-b baudrate]\n", 
                        *argv);
          exit( 1 );
    }

    printf("Opening device: vendor_id = %x, product_id = %x, serial_id = %s\n",
           vid, pid, sid );
    printf("  - interface = %d, baudrate = %d\n", interface, baudrate );
    
    if (ftdi_init(&ftdic) < 0)
    {
       fprintf(stderr, "ftdi_init failed\n");
       return EXIT_FAILURE;
    }

    // Select first interface
    ftdi_set_interface(&ftdic, interface);

    // Open device
    i = ftdi_usb_open_desc(&ftdic, vid, pid,NULL,sid);
    if (i < 0)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", i, 
                ftdi_get_error_string(&ftdic));
        exit(-1);
    }

    // Set baudrate
    i = ftdi_set_baudrate(&ftdic, baudrate);
    if (i < 0)
    {
        fprintf(stderr, "unable to set baudrate: %d (%s)\n", i, 
                ftdi_get_error_string(&ftdic));
        exit(-1);
    }

    printf("----------------Press 'x' to exit-----------------\n");

    //while ( ( key = getchar() ) != 'x' )
    while ( 1 )
    {
      if ((i = ftdi_read_data(&ftdic, (unsigned char *)buf, sizeof(buf))) >= 0) 
      {
        if ( i > 0 )
        {
          fprintf(stderr, "read %d bytes\n", i);
          fwrite(buf, i, 1, stdout);
          fflush(stderr);
          fflush(stdout);
        }
      }
      //reset_keypress();
    }

    if ((ret = ftdi_usb_close(&ftdic)) < 0)
    {
      fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, 
              ftdi_get_error_string(&ftdic));
      return EXIT_FAILURE;
    }

    ftdi_deinit(&ftdic);
  }

  return EXIT_SUCCESS;
}

