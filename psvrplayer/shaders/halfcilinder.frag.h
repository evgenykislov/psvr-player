unsigned char shaders_halfcilinder_frag[] = {
  0x23, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x33, 0x33, 0x30,
  0x20, 0x63, 0x6f, 0x72, 0x65, 0x0a, 0x0a, 0x23, 0x64, 0x65, 0x66, 0x69,
  0x6e, 0x65, 0x20, 0x4d, 0x5f, 0x50, 0x49, 0x20, 0x33, 0x2e, 0x31, 0x34,
  0x31, 0x35, 0x39, 0x32, 0x36, 0x35, 0x33, 0x35, 0x38, 0x39, 0x37, 0x39,
  0x33, 0x32, 0x33, 0x38, 0x34, 0x36, 0x32, 0x36, 0x34, 0x33, 0x33, 0x38,
  0x33, 0x32, 0x37, 0x39, 0x35, 0x0a, 0x0a, 0x6f, 0x75, 0x74, 0x20, 0x76,
  0x65, 0x63, 0x34, 0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x3b, 0x0a, 0x69,
  0x6e, 0x20, 0x76, 0x65, 0x63, 0x34, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65,
  0x5f, 0x70, 0x6f, 0x73, 0x3b, 0x0a, 0x75, 0x6e, 0x69, 0x66, 0x6f, 0x72,
  0x6d, 0x20, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x32, 0x44, 0x20,
  0x69, 0x6d, 0x61, 0x67, 0x65, 0x3b, 0x0a, 0x0a, 0x76, 0x6f, 0x69, 0x64,
  0x20, 0x6d, 0x61, 0x69, 0x6e, 0x28, 0x29, 0x0a, 0x7b, 0x0a, 0x20, 0x20,
  0x76, 0x65, 0x63, 0x34, 0x20, 0x70, 0x6f, 0x73, 0x20, 0x3d, 0x20, 0x73,
  0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f, 0x73, 0x3b, 0x0a, 0x20, 0x20,
  0x66, 0x6c, 0x6f, 0x61, 0x74, 0x20, 0x78, 0x5f, 0x61, 0x6e, 0x67, 0x6c,
  0x65, 0x3b, 0x0a, 0x20, 0x20, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x20, 0x79,
  0x5f, 0x61, 0x6e, 0x67, 0x6c, 0x65, 0x3b, 0x0a, 0x20, 0x20, 0x66, 0x6c,
  0x6f, 0x61, 0x74, 0x20, 0x6c, 0x5f, 0x68, 0x6f, 0x72, 0x3b, 0x20, 0x2f,
  0x2f, 0x21, 0x3c, 0x20, 0xd0, 0xa0, 0xd0, 0xb0, 0xd1, 0x81, 0xd1, 0x81,
  0xd1, 0x82, 0xd0, 0xbe, 0xd1, 0x8f, 0xd0, 0xbd, 0xd0, 0xb8, 0xd0, 0xb5,
  0x20, 0xd0, 0xb4, 0xd0, 0xbe, 0x20, 0xd0, 0xbe, 0xd1, 0x82, 0xd0, 0xbe,
  0xd0, 0xb1, 0xd1, 0x80, 0xd0, 0xb0, 0xd0, 0xb6, 0xd0, 0xb0, 0xd0, 0xb5,
  0xd0, 0xbc, 0xd0, 0xbe, 0xd0, 0xb9, 0x20, 0xd1, 0x82, 0xd0, 0xbe, 0xd1,
  0x87, 0xd0, 0xba, 0xd0, 0xb8, 0x20, 0xd0, 0xbf, 0xd0, 0xbe, 0x20, 0xd0,
  0xb3, 0xd0, 0xbe, 0xd1, 0x80, 0xd0, 0xb8, 0xd0, 0xb7, 0xd0, 0xbe, 0xd0,
  0xbd, 0xd1, 0x82, 0xd0, 0xb0, 0xd0, 0xbb, 0xd0, 0xb8, 0x0a, 0x0a, 0x20,
  0x20, 0x69, 0x66, 0x20, 0x28, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70,
  0x6f, 0x73, 0x2e, 0x7a, 0x20, 0x3e, 0x3d, 0x20, 0x30, 0x2e, 0x30, 0x66,
  0x29, 0x20, 0x7b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x2f, 0x2f, 0x20, 0xd0,
  0x92, 0xd0, 0xb8, 0xd0, 0xb4, 0x20, 0xd0, 0xb7, 0xd0, 0xb0, 0x20, 0xd1,
  0x81, 0xd0, 0xbf, 0xd0, 0xb8, 0xd0, 0xbd, 0xd0, 0xbe, 0xd0, 0xb9, 0x0a,
  0x20, 0x20, 0x20, 0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x3d, 0x20,
  0x76, 0x65, 0x63, 0x34, 0x28, 0x30, 0x2e, 0x30, 0x66, 0x2c, 0x20, 0x30,
  0x2e, 0x30, 0x66, 0x2c, 0x20, 0x30, 0x2e, 0x30, 0x66, 0x2c, 0x20, 0x30,
  0x2e, 0x30, 0x66, 0x29, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x72, 0x65,
  0x74, 0x75, 0x72, 0x6e, 0x3b, 0x0a, 0x20, 0x20, 0x7d, 0x0a, 0x0a, 0x20,
  0x20, 0x69, 0x66, 0x20, 0x28, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70,
  0x6f, 0x73, 0x2e, 0x78, 0x20, 0x3c, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65,
  0x5f, 0x70, 0x6f, 0x73, 0x2e, 0x7a, 0x29, 0x20, 0x7b, 0x0a, 0x20, 0x20,
  0x20, 0x20, 0x2f, 0x2f, 0x20, 0xd0, 0x9b, 0xd0, 0xb5, 0xd0, 0xb2, 0xd0,
  0xb0, 0xd1, 0x8f, 0x20, 0xd1, 0x87, 0xd0, 0xb0, 0xd1, 0x81, 0xd1, 0x82,
  0xd1, 0x8c, 0x2e, 0x20, 0xd0, 0x9e, 0xd1, 0x81, 0xd1, 0x8c, 0x20, 0x78,
  0x20, 0xd0, 0xbd, 0xd0, 0xb5, 0x20, 0xd1, 0x80, 0xd0, 0xb0, 0xd0, 0xb2,
  0xd0, 0xbd, 0xd0, 0xb0, 0x20, 0x30, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x78,
  0x5f, 0x61, 0x6e, 0x67, 0x6c, 0x65, 0x20, 0x3d, 0x20, 0x61, 0x74, 0x61,
  0x6e, 0x28, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f, 0x73, 0x2e,
  0x7a, 0x20, 0x2f, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f,
  0x73, 0x2e, 0x78, 0x29, 0x3b, 0x0a, 0x20, 0x20, 0x7d, 0x20, 0x65, 0x6c,
  0x73, 0x65, 0x20, 0x69, 0x66, 0x20, 0x28, 0x73, 0x63, 0x65, 0x6e, 0x65,
  0x5f, 0x70, 0x6f, 0x73, 0x2e, 0x78, 0x20, 0x3e, 0x20, 0x2d, 0x73, 0x63,
  0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f, 0x73, 0x2e, 0x7a, 0x29, 0x20, 0x7b,
  0x0a, 0x20, 0x20, 0x20, 0x20, 0x2f, 0x2f, 0x20, 0xd0, 0x9f, 0xd1, 0x80,
  0xd0, 0xb0, 0xd0, 0xb2, 0xd0, 0xb0, 0xd1, 0x8f, 0x20, 0xd1, 0x87, 0xd0,
  0xb0, 0xd1, 0x81, 0xd1, 0x82, 0xd1, 0x8c, 0x2e, 0x20, 0xd0, 0x9e, 0xd1,
  0x81, 0xd1, 0x8c, 0x20, 0x78, 0x20, 0xd0, 0xbd, 0xd0, 0xb5, 0x20, 0xd1,
  0x80, 0xd0, 0xb0, 0xd0, 0xb2, 0xd0, 0xbd, 0xd0, 0xb0, 0x20, 0x30, 0x0a,
  0x20, 0x20, 0x20, 0x20, 0x78, 0x5f, 0x61, 0x6e, 0x67, 0x6c, 0x65, 0x20,
  0x3d, 0x20, 0x4d, 0x5f, 0x50, 0x49, 0x20, 0x2d, 0x20, 0x61, 0x74, 0x61,
  0x6e, 0x28, 0x2d, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f, 0x73,
  0x2e, 0x7a, 0x20, 0x2f, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70,
  0x6f, 0x73, 0x2e, 0x78, 0x29, 0x3b, 0x0a, 0x20, 0x20, 0x7d, 0x20, 0x65,
  0x6c, 0x73, 0x65, 0x20, 0x7b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x2f, 0x2f,
  0x20, 0xd0, 0xa6, 0xd0, 0xb5, 0xd0, 0xbd, 0xd1, 0x82, 0xd1, 0x80, 0xd0,
  0xb0, 0xd0, 0xbb, 0xd1, 0x8c, 0xd0, 0xbd, 0xd0, 0xb0, 0xd1, 0x8f, 0x20,
  0xd1, 0x87, 0xd0, 0xb0, 0xd1, 0x81, 0xd1, 0x82, 0xd1, 0x8c, 0x2e, 0x20,
  0xd0, 0x9e, 0xd1, 0x81, 0xd1, 0x8c, 0x20, 0x7a, 0x20, 0xd0, 0xbd, 0xd0,
  0xb5, 0x20, 0xd1, 0x80, 0xd0, 0xb0, 0xd0, 0xb2, 0xd0, 0xbd, 0xd0, 0xb0,
  0x20, 0x30, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x78, 0x5f, 0x61, 0x6e, 0x67,
  0x6c, 0x65, 0x20, 0x3d, 0x20, 0x4d, 0x5f, 0x50, 0x49, 0x20, 0x2f, 0x20,
  0x32, 0x2e, 0x30, 0x66, 0x20, 0x2b, 0x20, 0x61, 0x74, 0x61, 0x6e, 0x28,
  0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f, 0x73, 0x2e, 0x78, 0x20,
  0x2f, 0x20, 0x2d, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f, 0x73,
  0x2e, 0x7a, 0x29, 0x3b, 0x0a, 0x20, 0x20, 0x7d, 0x0a, 0x0a, 0x20, 0x20,
  0x6c, 0x5f, 0x68, 0x6f, 0x72, 0x20, 0x3d, 0x20, 0x73, 0x71, 0x72, 0x74,
  0x28, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f, 0x73, 0x2e, 0x78,
  0x20, 0x2a, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f, 0x73,
  0x2e, 0x78, 0x20, 0x2b, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70,
  0x6f, 0x73, 0x2e, 0x7a, 0x20, 0x2a, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65,
  0x5f, 0x70, 0x6f, 0x73, 0x2e, 0x7a, 0x29, 0x3b, 0x0a, 0x20, 0x20, 0x79,
  0x5f, 0x61, 0x6e, 0x67, 0x6c, 0x65, 0x20, 0x3d, 0x20, 0x4d, 0x5f, 0x50,
  0x49, 0x20, 0x2f, 0x20, 0x32, 0x2e, 0x30, 0x66, 0x20, 0x2b, 0x20, 0x61,
  0x74, 0x61, 0x6e, 0x28, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x5f, 0x70, 0x6f,
  0x73, 0x2e, 0x79, 0x20, 0x2f, 0x20, 0x6c, 0x5f, 0x68, 0x6f, 0x72, 0x29,
  0x3b, 0x0a, 0x0a, 0x20, 0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x3d,
  0x20, 0x74, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x28, 0x69, 0x6d, 0x61,
  0x67, 0x65, 0x2c, 0x20, 0x76, 0x65, 0x63, 0x32, 0x28, 0x78, 0x5f, 0x61,
  0x6e, 0x67, 0x6c, 0x65, 0x20, 0x2f, 0x20, 0x4d, 0x5f, 0x50, 0x49, 0x20,
  0x2c, 0x20, 0x79, 0x5f, 0x61, 0x6e, 0x67, 0x6c, 0x65, 0x20, 0x2f, 0x20,
  0x4d, 0x5f, 0x50, 0x49, 0x29, 0x29, 0x3b, 0x0a, 0x7d, 0x0a
};
unsigned int shaders_halfcilinder_frag_len = 1066;
