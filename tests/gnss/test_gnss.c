#include "gnss_i2c.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t sent[17];
static uint16_t sent_size;
static HAL_StatusTypeDef tx_status = HAL_OK;
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef*h,uint16_t a,uint32_t n,uint32_t t) { (void)h;(void)a;(void)n;(void)t;return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef*h,uint16_t a,uint16_t r,uint16_t z,uint8_t*b,uint16_t n,uint32_t t) { (void)h;(void)a;(void)r;(void)z;(void)b;(void)n;(void)t;return HAL_ERROR; }
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef*h,uint16_t a,uint8_t*b,uint16_t n,uint32_t t) { (void)h;(void)a;(void)b;(void)n;(void)t;return HAL_ERROR; }
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef*h,uint16_t a,uint8_t*b,uint16_t n,uint32_t t) { assert(h);assert(a==0x84);assert(t==100);sent_size=n;memcpy(sent,b,n);return tx_status; }
static void checksum(uint8_t *p, size_t n) { uint8_t a=0,b=0;for(size_t i=2;i<n-2;i++){a+=p[i];b+=a;}p[n-2]=a;p[n-1]=b; }
static unsigned feed(GNSS_UBXParser*p,const uint8_t*b,size_t n) { unsigned hits=0;for(size_t i=0;i<n;i++)hits+=GNSS_I2C_AirborneVerified(p,b[i]);return hits; }
int main(void) {
 I2C_HandleTypeDef h={0};
 const uint8_t set[]={0xb5,0x62,6,0x8a,9,0,0,1,0,0,0x21,0,0x11,0x20,6,0xf2,0x4f};
 assert(GNSS_I2C_SetAirborne(&h)==HAL_OK);assert(sent_size==sizeof(set));assert(!memcmp(sent,set,sizeof(set)));
 assert(GNSS_I2C_RequestDynamicModel(&h)==HAL_OK);assert(sent_size==16);assert(sent[3]==0x8b && sent[6]==0 && sent[7]==0);uint8_t expected[17];memcpy(expected,sent,16);checksum(expected,16);assert(!memcmp(sent,expected,16));
 tx_status=HAL_TIMEOUT;assert(GNSS_I2C_SetAirborne(&h)==HAL_TIMEOUT);assert(GNSS_I2C_SetAirborne(NULL)==HAL_ERROR);
 uint8_t reply[]={0xb5,0x62,6,0x8b,9,0,1,0,0,0,0x21,0,0x11,0x20,6,0,0};checksum(reply,sizeof(reply));
 GNSS_UBXParser p={0};assert(feed(&p,(const uint8_t*)"$GNGGA,noise\r\n",14)==0);assert(feed(&p,reply,7)==0);assert(feed(&p,reply+7,sizeof(reply)-7)==1);
 reply[16]^=1;assert(feed(&p,reply,sizeof(reply))==0);reply[16]^=1;
 for(unsigned i=6;i<=14;i++){uint8_t saved=reply[i];reply[i]^=1;checksum(reply,sizeof(reply));assert(feed(&p,reply,sizeof(reply))==0);reply[i]=saved;checksum(reply,sizeof(reply));}
 uint8_t other[80]={0xb5,0x62,1,7,72,0};memcpy(other+8,reply,sizeof(reply));checksum(other,sizeof(other));assert(feed(&p,other,sizeof(other))==0);assert(feed(&p,reply,sizeof(reply))==1);
 assert(GNSS_I2C_AirborneVerified(NULL,0)==0);puts("GNSS tests passed");
}
