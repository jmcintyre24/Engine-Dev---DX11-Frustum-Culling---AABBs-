#ifdef __cplusplus
#define cbuffer struct
#define matrix DirectX::XMMATRIX
#endif

cbuffer MVP_t
{
	matrix modeling;
	matrix view;
	matrix projection;
};
