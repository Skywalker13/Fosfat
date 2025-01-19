
#ifndef FOSGRA_BMP_H
#define FOSGRA_BMP_H

size_t get_bmp1_size (int width, int height, int *bpr, int *pbpr);
size_t get_bmp4_size (int width, int height, int *bpr, int *image_size, int *header_size);

void
create_bmp1_buffer(const uint8_t *input,
                   int width, int height, uint8_t **output, size_t *output_size);
void
create_bmp4_buffer(const uint8_t *input, const uint32_t *pal,
                   int width, int height, uint8_t **output, size_t *output_size);

#endif /* FOSGRA_BMP_H */
