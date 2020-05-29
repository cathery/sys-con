#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define BUFFER_SIZE 7

static unsigned char buffer[BUFFER_SIZE] = {0};

struct button_map
{
    unsigned char index;
    unsigned char position;
};

enum hat_state
{
    HAT_UP,
    HAT_UPRIGHT,
    HAT_RIGHT,
    HAT_DOWNRIGHT,
    HAT_DOWN,
    HAT_DOWNLEFT,
    HAT_LEFT,
    HAT_UPLEFT,
    HAT_UNPRESSED,
};

static struct button_map buttonMaps[] =
{
    {4, 5},
    {4, 6},
    {4, 7},
    {4, 4},
    {5, 0},
    {5, 1},
    {5, 2},
    {5, 3},
    {5, 4},
    {5, 5},
    {6, 0},
    {5, 6},
    {5, 7},
    {4, 1},
    {4, 3},
    {4, 0},
    {4, 2},
};

int axisToIndexMap[] =
{
    0,
    1,
    -1,
    2,
    3,
};

int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

int main(int argc, char *argv[])
{
    char device[] = "/dev/input/js0";
    int js, gd, yes = 1;
    struct js_event event;
    struct sockaddr_in saddr;

    if(argc != 3)
    {
        printf("\n Usage: %s <ip of server> <js number>\n",argv[0]);
        return 1;
    } 

    device[13] = argv[2][0];

    js = open(device, O_RDONLY);

    if (js == -1)
        perror("Could not open joystick");

    gd = socket(AF_INET, SOCK_STREAM, 0);
    if (gd == -1)
        perror("Could not open socket");

    memset(&saddr, 0, sizeof(saddr));

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8080);

    if (inet_pton(AF_INET, argv[1], &saddr.sin_addr) <= 0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    if (connect(gd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

    if (setsockopt(gd, IPPROTO_TCP, TCP_NODELAY, (void *)&yes, sizeof(yes)) < 0)
    {
        printf("\n Error : TCP NODELAY Failed \n");
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));

    // Init axis at rest
    buffer[0] = 127;
    buffer[1] = 127;
    buffer[2] = 127;
    buffer[3] = 127;
     
    /* This loop will exit if the controller is unplugged. */
    while (read_event(js, &event) == 0)
    {
        switch (event.type)
        {
            case JS_EVENT_BUTTON:
                printf("Button %u %s\n", event.number, event.value ?
                                                       "pressed" : "released");

                struct button_map map = buttonMaps[event.number];
                if (event.value)
                {
                    buffer[map.index] |= 1 << map.position; 
                }
                else
                {
                    buffer[map.index] &= ~(1 << map.position); 
                }

                write(gd, buffer, BUFFER_SIZE);
                break;
            case JS_EVENT_AXIS:
                printf("Axis %zu value %6d\n", event.number, event.value);

                switch (event.number)
                {
                    case 6:
                    case 7:
                    {
                        int buttonNumber, buttonValue = 1;
                        buttonNumber = (event.number == 6 && event.value < 0) ? 15 :
                                       (event.number == 6 && event.value > 0) ? 16 :
                                       (event.number == 7 && event.value < 0) ? 13 :
                                       (event.number == 7 && event.value > 0) ? 14 : -1;

                        if (buttonNumber == -1) 
                        {
                            int i;
                            for (i = 0; i < 2; i++)
                            {
                                struct button_map map = buttonMaps[(event.number == 7 ? 13 : 15) + i];
                                buffer[map.index] &= ~(1 << map.position);
                            }
                        }
                        else
                        {
                            struct button_map map = buttonMaps[buttonNumber];
                            buffer[map.index] |= 1 << map.position; 
                        }
                    }
                    break;

                    case 0:
                    case 1:
                    case 3:
                    case 4:
                    {
                        int index = axisToIndexMap[event.number];
                        buffer[index] = (unsigned char)
                                            (((int)event.value + 32767) >> 8);
                        printf("Normalized value %3d\n",
                                                (unsigned char)buffer[index]);
                    }
                    break;
                }
                write(gd, buffer, BUFFER_SIZE);
                break;

            default:
                /* Ignore init events. */
                printf("Weird event: %d\n", event.type);
                break;
        }

        fflush(stdout);
    }

    close(js);
    return 0;
}
