#ifndef _NBASE64_H
#define _NBASE64_H

class NBase64Class{
  public:
    int encode(char *output, char *input, int inputLength);
    int decode(char * output, char * input, int inputLength);
    int encodedLength(int plainLength);
    int decodedLength(char * input, int inputLength);

  private:
    inline void fromA3ToA4(unsigned char * A4, unsigned char * A3);
    inline void fromA4ToA3(unsigned char * A3, unsigned char * A4);
    inline unsigned char lookupTable(char c);
};
extern NBase64Class NBase64;

#endif // _NBASE64_H
