#include <rsx/commands.h>
#include <rsx/nv40.h>
#include <rsx/buffer.h>
#include <rsx/reality.h>

void realitySetClearColor(gcmContextData *context, uint32_t color) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_CLEAR_COLOR_VALUE, color);
}

void realitySetClearDepthValue(gcmContextData *context, uint32_t value) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_CLEAR_DEPTH_VALUE, value);
}

void realityNop(gcmContextData *context) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, 0x100, 0);
}

void realityClearBuffers(gcmContextData *context, uint32_t buffers) {
	COMMAND_LENGTH(context, 4);
	commandBufferPutCmd1(context, NV30_3D_CLEAR_BUFFERS, buffers);
	commandBufferPutCmd1(context, 0x100, 0); // Nop
}	

const static uint32_t offset_cmds[] = {NV30_3D_COLOR0_OFFSET, NV30_3D_COLOR1_OFFSET,
		NV40_3D_COLOR2_OFFSET, NV40_3D_COLOR3_OFFSET, NV30_3D_ZETA_OFFSET};
const static uint32_t dma_cmds[] = {NV30_3D_DMA_COLOR0, NV30_3D_DMA_COLOR1,
		NV40_3D_DMA_COLOR2, NV40_3D_DMA_COLOR3, NV30_3D_DMA_ZETA};
const static uint32_t pitch_cmds[] = {NV30_3D_COLOR0_PITCH, NV30_3D_COLOR1_PITCH,
		NV40_3D_COLOR2_PITCH, NV40_3D_COLOR3_PITCH, NV40_3D_ZETA_PITCH};

void realitySetRenderSurface(gcmContextData *context, uint8_t surface, uint8_t location,
				uint32_t offset, uint32_t pitch) {
	COMMAND_LENGTH(context, 6);
	commandBufferPutCmd1(context, dma_cmds[surface], location | 0xFEED0000);
	commandBufferPutCmd1(context, offset_cmds[surface], offset);
	commandBufferPutCmd1(context, pitch_cmds[surface], pitch);
}

void realitySelectRenderTarget(gcmContextData *context, uint8_t target, uint32_t format, 
				uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
	COMMAND_LENGTH(context, 11);

	format |= (31 - __builtin_clz(width)) << 16;
	format |= (31 - __builtin_clz(height)) << 24;
	uint32_t conventions = height | 0x1000; // COORD_CONVENTIONS_ORIGIN_INVERTED
	
	commandBufferPutCmd1(context, NV30_3D_RT_FORMAT, format); 
	commandBufferPutCmd1(context, NV30_3D_RT_ENABLE, target);
	commandBufferPutCmd1(context, NV30_3D_VIEWPORT_TX_ORIGIN, x | y << 16);
	commandBufferPutCmd1(context, NV30_3D_COORD_CONVENTIONS, conventions);
	commandBufferPutCmd2(context, NV30_3D_RT_HORIZ, x | width << 16, y | height << 16);
}

void realityViewport(gcmContextData *context, uint16_t width, uint16_t height) {
	COMMAND_LENGTH(context, 3);
	commandBufferPutCmd2(context, NV30_3D_VIEWPORT_HORIZ, width << 16, height << 16);
}

void realityVertexBegin(gcmContextData *context, uint32_t type) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, type);
}

void realityVertexEnd(gcmContextData *context) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, NV30_3D_VERTEX_BEGIN_END_STOP);
}

void realityVertex4f(gcmContextData *context, float x, float y, float z, float w) {
	COMMAND_LENGTH(context, 5);
	commandBufferPutCmd4(context, NV30_3D_VTX_ATTR_4F(0),
				 ((ieee32) x).u, 
				 ((ieee32) y).u,
				 ((ieee32) z).u, 
				 ((ieee32) w).u);
}

void realityTexCoord2f(gcmContextData *context, float s, float t) {
	COMMAND_LENGTH(context, 3);
	commandBufferPutCmd2(context, NV30_3D_VTX_ATTR_2F(8),
				 ((ieee32) s).u, 
				 ((ieee32) t).u);
}


void realityLoadVertexProgram(gcmContextData *context, realityVertexProgram *prog) {
	int inst, command_size = prog->NumInsts * 5 + 7;
	unsigned int *ucode = (unsigned int*)realityVertexProgramGetUCode(prog);
	realityVertexProgramConstant *constants;
	COMMAND_LENGTH(context, command_size);
	
	commandBufferPutCmd1(context, NV30_3D_VP_UPLOAD_FROM_ID, 0);
	
	for(inst = 0; inst < prog->NumInsts*4; inst += 4) {
		commandBufferPutCmd4(context, NV30_3D_VP_UPLOAD_INST(inst), 
					ucode[inst + 0],
					ucode[inst + 1],
					ucode[inst + 2],
					ucode[inst + 3]);
	}

	commandBufferPutCmd1(context, NV30_3D_VP_START_FROM_ID, 0);
	commandBufferPutCmd2(context, NV40_3D_VP_ATTRIB_EN, prog->InputMask, prog->OutputMask);

	constants = realityVertexProgramGetConstants(prog);

	if(constants)
	{
		int c;
		for(c=0;c<prog->NumConstants;++c)
		{
			realitySetVertexProgramConstant4f(context,constants[c].Index,(float*)constants[c].Values);
		}
	}
}

void realitySetVertexProgramConstant4f(gcmContextData *context, int num, float values[4]){

	COMMAND_LENGTH(context, 6);

	commandBufferPutCmd5(context, NV30_3D_VP_UPLOAD_CONST_ID, 
							num,
							((ieee32) (values[0])).u,
							((ieee32) (values[1])).u,
							((ieee32) (values[2])).u,
							((ieee32) (values[3])).u
							);
}

void realitySetVertexProgramConstant4fBlock(gcmContextData *context, int constant, int num4fConstants, float *values)
{
	int n;
	for(n=0;n<num4fConstants;++n)
	{
		realitySetVertexProgramConstant4f(context,constant+n,values+4*n);
	}
}

void realityInstallFragmentProgram(gcmContextData *context, realityFragmentProgram *prog, uint32_t *addr) {
	// We don't actually need context, but if we leave it out people will forget.
	int i;
	for( i = 0; i < prog->size; ++i ) {
		addr[i] = (((prog->data[i] >> 16 ) & 0xffff) << 0) |
			    (((prog->data[i] >> 0 ) & 0xffff) << 16);
	}
	assert(realityAddressToOffset(addr, &prog->offset) == 0);
}

void realityLoadFragmentProgram(gcmContextData *context, realityFragmentProgram *prog) {
	COMMAND_LENGTH(context, 4);
	assert(prog->offset != 0);
	commandBufferPutCmd1(context, NV30_3D_FP_ACTIVE_PROGRAM,
				prog->offset | NV30_3D_FP_ACTIVE_PROGRAM_DMA0);
	commandBufferPutCmd1(context, NV30_3D_FP_CONTROL, 
 				prog->num_regs << NV40_3D_FP_CONTROL_TEMP_COUNT__SHIFT);
}


void realitySetTexture(gcmContextData *context, uint32_t unit, realityTexture *tex) {
	COMMAND_LENGTH(context, 11);
	commandBufferPutCmd8(context, NV30_3D_TEX_OFFSET(unit), tex->offset, tex->format,
		tex->wrap, tex->enable, tex->swizzle, tex->filter,
		tex->width << 16 | tex->height, tex->borderColor);
	commandBufferPutCmd1(context, NV40_3D_TEX_SIZE1(unit), tex->stride | 
		(1 << NV40_3D_TEX_SIZE1_DEPTH__SHIFT));
}

void realityViewportTranslate(gcmContextData *context, float x, float y, float z, float w) {
	COMMAND_LENGTH(context, 5);
	commandBufferPutCmd4(context, NV30_3D_VIEWPORT_TRANSLATE, 
				 ((ieee32) x).u, 
				 ((ieee32) y).u,
				 ((ieee32) z).u, 
				 ((ieee32) w).u);
}

void realityViewportScale(gcmContextData *context, float x, float y, float z, float w) {
	COMMAND_LENGTH(context, 5);
	commandBufferPutCmd4(context, NV30_3D_VIEWPORT_SCALE, 
				 ((ieee32) x).u,
				 ((ieee32) y).u,
				 ((ieee32) z).u, 
				 ((ieee32) w).u);
};

void realityCullFace(gcmContextData *context, uint32_t face) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_CULL_FACE, face);
}

void realityFrontFace(gcmContextData *context, uint32_t face) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_FRONT_FACE, face);
}

void realityCullEnable(gcmContextData *context, uint32_t enable) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_CULL_FACE_ENABLE, enable);
}

void realityBlendEnable(gcmContextData *context, uint32_t enable) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_BLEND_FUNC_ENABLE, enable);
}

void realityBlendEquation(gcmContextData *context, uint32_t equation) {
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV40_3D_BLEND_EQUATION, equation);
}

void realityBlendFunc(gcmContextData *context, uint32_t src, uint32_t dest) {
	COMMAND_LENGTH(context, 3);
	commandBufferPutCmd2(context, NV30_3D_BLEND_FUNC_SRC, src, dest);
}

void realityZControl(gcmContextData *context, uint8_t cullNearFar, uint8_t zClampEnable, uint8_t cullIngnoreW) { // todo: documenent in nouveau
	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_DEPTH_CONTROL, cullNearFar | 
				zClampEnable << 4 |
				cullIngnoreW << 8);
}

void realityBindVertexBufferAttribute(gcmContextData *context, uint8_t attribute, uint32_t offset, uint8_t stride, uint8_t elements, uint8_t dataType, uint8_t location)
{
	COMMAND_LENGTH(context, 2+2);
	
	commandBufferPutCmd1(context, NV30_3D_VTXBUF(attribute), offset|(location<<31));
	commandBufferPutCmd1(context, NV30_3D_VTXFMT(attribute), (stride<<NV30_3D_VTXFMT_STRIDE__SHIFT)|(elements<<NV30_3D_VTXFMT_SIZE__SHIFT)|dataType);

}

void realityDrawVertexBuffer(gcmContextData *context, uint32_t type, uint32_t start, uint32_t count)
{
	COMMAND_LENGTH(context, 4);

	commandBufferPutCmd1(context, NV40_3D_VTX_CACHE_INVALIDATE, 0);
	commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, type);

	while(count)
	{
		int num=count; //number of elements this call
		if(num>256)	//max 256 elements per call
			num=256;
		COMMAND_LENGTH(context, 2);
		commandBufferPutCmd1(context, NV30_3D_VB_VERTEX_BATCH, ((num-1)<<24)|start);
		count-=num;
		start+=num;
	}

	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, NV30_3D_VERTEX_BEGIN_END_STOP);
}

void realityDrawVertexBufferIndex(gcmContextData *context, uint32_t type, uint32_t offset, uint32_t count, uint8_t dataType, uint8_t location)
{
	unsigned int current=0;

	COMMAND_LENGTH(context, 7);

	commandBufferPutCmd1(context, NV40_3D_VTX_CACHE_INVALIDATE, 0);

	commandBufferPutCmd2(context, NV30_3D_IDXBUF_OFFSET, offset, (uint32_t)(dataType|location));

	commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, type);

	while(count)
	{
		int num=count; //number of elements this call
		if(num>256)	//max 256 elements per call
			num=256;		
		COMMAND_LENGTH(context, 2);
		commandBufferPutCmd1(context, NV30_3D_VB_INDEX_BATCH, ((num-1)<<24)|current);
		count-=num;
		current+=num;
	}

	COMMAND_LENGTH(context, 2);
	commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, NV30_3D_VERTEX_BEGIN_END_STOP);
}

void realityDepthTestEnable(gcmContextData *context, uint32_t enable)
{
	COMMAND_LENGTH(context,2);
	commandBufferPutCmd1(context, NV30_3D_DEPTH_TEST_ENABLE, enable);
}

void realityDepthTestFunc(gcmContextData *context, uint32_t function)
{
	COMMAND_LENGTH(context,2);
	commandBufferPutCmd1(context, NV30_3D_DEPTH_FUNC, function);
}

void realityDepthWriteEnable(gcmContextData *context, uint32_t enable)
{
	COMMAND_LENGTH(context,2);
	commandBufferPutCmd1(context, NV30_3D_DEPTH_WRITE_ENABLE, enable);
}
