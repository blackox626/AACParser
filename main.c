#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int getADTSframe(unsigned char *buffer, int buf_size, unsigned char *data, int *data_size) {
    int size = 0;

    if (!buffer || !data || !data_size) {
        return -1;
    }

    while (1) {
        if (buf_size < 7) {
            return -1;
        }

        /// size : adtsframe的长度 包括 Sync words(同步字 0xFFF)

        //Sync words
        if ((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0)) {
            size |= ((buffer[3] & 0x03) << 11);     //high 2 bit
            size |= buffer[4] << 3;                //middle 8 bit
            size |= ((buffer[5] & 0xe0) >> 5);        //low 3bit
            break;
        }
        --buf_size;
        ++buffer;
    }

    if (buf_size < size) {
        return 1;
    }

    memcpy(data, buffer, size);
    *data_size = size;

    return 0;
}

int simplest_aac_parser(char *url) {
    int data_size = 0;
    int size = 0;
    int cnt = 0;
    int offset = 0;

    //FILE *myout=fopen("output_log.txt","wb+");
    FILE *myout = stdout;

    unsigned char *aacframe = (unsigned char *) malloc(1024 * 5);
    unsigned char *aacbuffer = (unsigned char *) malloc(1024 * 1024);

    FILE *ifile = fopen(url, "rb");
    if (!ifile) {
        printf("Open file error");
        return -1;
    }

    printf("------------------+- ADTS Frame Table -+-----------------------+\n");
    printf(" NUM | Len | Profile | Frequency| Channel | Sample Count |Size |\n");
    printf("-----+-----+---------+----------+---------+--------------+------\n");

    while (!feof(ifile)) {
        data_size = fread(aacbuffer + offset, 1, 1024 * 1024 - offset, ifile);
        unsigned char *input_data = aacbuffer;

        while (1) {
            int ret = getADTSframe(input_data, data_size, aacframe, &size);
            if (ret == -1) {
                break;
            } else if (ret == 1) {
                memcpy(aacbuffer, input_data, data_size);
                offset = data_size;
                break;
            }

            char head_len_str[10] = {0};

            char profile_str[10] = {0};
            char frequence_str[10] = {0};

            //当protection_absend为1时表示无crc, ADTS可变头长度 为28bit,为0时，可变头长度为44bit.
            if((aacframe[1] & 0x01) == 0) {
                sprintf(head_len_str,"len 9");
            } else {
                sprintf(head_len_str,"len 7");
            }

            /// 1100 0000  C0
            unsigned char profile = aacframe[2] & 0xC0;
            /// 前两位
            profile = profile >> 6;
            switch (profile) {
                case 0:
                    sprintf(profile_str, "Main");
                    break;
                case 1:
                    sprintf(profile_str, "LC");
                    break;
                case 2:
                    sprintf(profile_str, "SSR");
                    break;
                default:
                    sprintf(profile_str, "unknown");
                    break;
            }
            /// 0011 1100  3C
            unsigned char sampling_frequency_index = aacframe[2] & 0x3C;
            /// 中间4位
            sampling_frequency_index = sampling_frequency_index >> 2;
            switch (sampling_frequency_index) {
                case 0:
                    sprintf(frequence_str, "96000Hz");
                    break;
                case 1:
                    sprintf(frequence_str, "88200Hz");
                    break;
                case 2:
                    sprintf(frequence_str, "64000Hz");
                    break;
                case 3:
                    sprintf(frequence_str, "48000Hz");
                    break;
                case 4:
                    sprintf(frequence_str, "44100Hz");
                    break;
                case 5:
                    sprintf(frequence_str, "32000Hz");
                    break;
                case 6:
                    sprintf(frequence_str, "24000Hz");
                    break;
                case 7:
                    sprintf(frequence_str, "22050Hz");
                    break;
                case 8:
                    sprintf(frequence_str, "16000Hz");
                    break;
                case 9:
                    sprintf(frequence_str, "12000Hz");
                    break;
                case 10:
                    sprintf(frequence_str, "11025Hz");
                    break;
                case 11:
                    sprintf(frequence_str, "8000Hz");
                    break;
                default:
                    sprintf(frequence_str, "unknown");
                    break;
            }

            /// channel_configuration: 表示声道数   0 是啥？
            //0: Defined in AOT Specifc Config (Audio Object Type)
            //1: 1 channel: front-center
            //2: 2 channels: front-left, front-right
            //3: 3 channels: front-center, front-left, front-right
            //4: 4 channels: front-center, front-left, front-right, back-center
            //5: 5 channels: front-center, front-left, front-right, back-left, back-right
            //6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
            //7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
            //8-15: Reserved
            int channel_configuration = (aacframe[2] & 0x01) << 2 | (aacframe[3] & 0xC0) >> 6;

            int adts_buffer_fullness = ((aacframe[5] & 0x1F) << 6) | ((aacframe[6] & 0xFC) >> 2);        //low 3bit

            int sample_count = (aacframe[6] & 0x03);

            fprintf(myout, "%5d| %8s| %8s|  %8s| %5d| %5d| %5d|\n", cnt,head_len_str, profile_str, frequence_str,channel_configuration,sample_count, size);
            data_size -= size;
            input_data += size;
            cnt++;
        }

    }
    fclose(ifile);
    free(aacbuffer);
    free(aacframe);

    return 0;
}

int main() {

    char *aacpath = "/Users/blackox626/CLionProjects/AACParser/resource/audio.aac";
    simplest_aac_parser(aacpath);

    return 0;
}
