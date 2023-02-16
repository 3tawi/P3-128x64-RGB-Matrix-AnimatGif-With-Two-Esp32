/*
// - Lien vidéo: https://youtu.be/wb8hE6A5osU
// REQUIRES the following Arduino libraries:
// - AnimatedGIF Library:  https://github.com/bitbank2/AnimatedGIF
*/

#include <SD.h>
#include <AnimatedGIF.h> 

const uint16_t kMatrixWidth = 128; 
const uint16_t kMatrixHeight = 64;

AnimatedGIF gif;
File f;
Stream* mySeriel;

#define UpHeader 0x9C
#define endHeader 0x36 

const uint16_t NUM_LEDS = kMatrixWidth * kMatrixHeight;
uint16_t usPalette[255];
uint8_t buff[NUM_LEDS];


void setDriver(Stream* s) {
  mySeriel = s;
}

void updateScreen() {
  mySeriel->write(UpHeader);
  mySeriel->write((uint8_t *)usPalette, 255*2);
  mySeriel->write((uint8_t *)buff, NUM_LEDS);
  mySeriel->write(endHeader);
}

// Draw a line of image directly on the LED Matrix
void GIFDraw(GIFDRAW *pDraw) {
  int y = (pDraw->iY + pDraw->y), xy; 
  if (pDraw->y ==0){
    memcpy(usPalette, (uint8_t *)pDraw->pPalette, 510);
  }
  xy = (kMatrixWidth * y) + pDraw->iX;
  memcpy((buff+xy), (uint8_t *)pDraw->pPixels, pDraw->iWidth);
} /* GIFDraw() */

void * GIFOpenFile(const char *fname, int32_t *pSize)
{
  //Serial.print("Playing gif: ");
  //Serial.println(fname);
  f = SD.open(fname);
  if (f)
  {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
} /* GIFOpenFile() */

void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
     f->close();
} /* GIFCloseFile() */

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0)
       return 0;
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    return iBytesRead;
} /* GIFReadFile() */

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{ 
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
//  Serial.printf("Seek time = %d us\n", i);
  return pFile->iPos;
} /* GIFSeekFile() */

unsigned long start_tick = 0;

void ShowGIF(char *name)
{
  start_tick = millis();
   
  if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
      while (gif.playFrame(true, NULL))
      {
         mySeriel->write(UpHeader);
         mySeriel->write((uint8_t *)usPalette, 510);
         mySeriel->write((uint8_t *)buff, NUM_LEDS);
         mySeriel->write(endHeader);
         memset(buff, 0xFF, NUM_LEDS);
         // yield;
       // updateScreen();
      }
    gif.close();
  }

} /* ShowGIF() */


void setup() {
  Serial.begin(1300000);
  delay(5000);
  setDriver(&Serial);
  Serial.println(" * Loading SD");
  if(!SD.begin(3)){
        Serial.println("SD Mount Failed");
  }
  gif.begin(LITTLE_ENDIAN_PIXELS);
  //gif.begin(LITTLE_ENDIAN_PIXELS);
  //gif.begin(BIG_ENDIAN_PIXELS, GIF_PALETTE_RGB888);
}

String gifDir = "/gifs"; // play all GIFs in this directory on the SD card
char filePath[256] = { 0 };
File root, gifFile;

void loop() 
{  
      root = SD.open(gifDir);
      if (root)
      {
           gifFile = root.openNextFile();
            while (gifFile) {              
              memset(filePath, 0x0, sizeof(filePath));                
              strcpy(filePath, gifFile.name());
              ShowGIF(filePath);
              gifFile.close();
              gifFile = root.openNextFile();
            }
         root.close();
      } // root
      
      delay(4000); // pause before restarting
      
}
