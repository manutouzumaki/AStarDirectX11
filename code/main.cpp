#include <d3d11.h>
#include <d3dx11.h>
#include <DxErr.h>
#include <d3dcompiler.h>

#include <Windows.h>
#include <stdio.h>

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabyytes(Value)*1024LL)
#define Terabyytes(Value) (terabytes(Value)*1024LL)

#define Assert(condition) if(!(condition)) { *(unsigned int *)0 = 0; } 
#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#include "math.h"
#include "Arena.h"
#include "AStar.h"

#include "Arena.cpp"
#include "AStar.cpp"



// Vertex Shader
static char *VertexShaderSource =
"cbuffer CBufferProjection : register(b0)\n"
"{\n"
"   matrix Projection;\n"
"   matrix World;\n"
"   float3 Color;\n"
"};\n"
"struct VS_Input\n"
"{\n"
"   float4 pos : POSITION;\n"
"   float2 tex0 : TEXCOORD0;\n"
"};\n"
"struct PS_Input\n"
"{\n"
"   float4 pos : SV_POSITION;\n"
"   float2 tex0 : TEXCOORD0;\n"
"   float3 tex1 : TEXCOORD1;\n"
"};\n"
"PS_Input VS_Main( VS_Input vertex )\n"
"{\n"
"   PS_Input vsOut = ( PS_Input )0;\n"
"   vsOut.pos = mul(vertex.pos, World);\n"
"   vsOut.pos = mul(vsOut.pos, Projection);\n"
"   vsOut.tex0 = vertex.tex0;\n"
"   vsOut.tex1 = Color;\n"
"   return vsOut;\n"
"}\0";




// Pixel Shader
static char *PixelShaderSource  =
"Texture2D colorMap_ : register( t0 );\n"
"SamplerState colorSampler_ : register( s0 );\n"
"struct PS_Input\n"
"{\n"
"   float4 pos : SV_POSITION;\n"
"   float2 tex0 : TEXCOORD0;\n"
"   float3 tex1 : TEXCOORD1;\n"
"};\n"
"float4 PS_Main( PS_Input frag ) : SV_TARGET\n"
"{\n"
"   return float4(frag.tex1.rgb, 1);\n"
"}\0";



static int
StringLength(char * String)
{
    int Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return Count;
}

struct vertex_pos
{
    v3 Pos;
    v2 Tex;
};

// We are going to use this buffer to pass the world and projection matix top the 
// vestex shader
struct VS_CONSTANT_BUFFER
{
    mat4 Projection;
    mat4 World;
    v3 Color;
};

VS_CONSTANT_BUFFER GlobalConstantBuffer;
static ID3D11Buffer *GlobalBuffer = 0;
ID3D11Buffer *VertexBuffer = 0;

static ID3D11VertexShader *VertexShader = 0;
static ID3D11PixelShader  *PixelShader  = 0;
static ID3D11InputLayout  *InputLayout  = 0;

static bool GlobalRunning;

void SetProjectionMat4(ID3D11DeviceContext *RenderContext, mat4 Projection)
{
    GlobalConstantBuffer.Projection = Projection;
    D3D11_MAPPED_SUBRESOURCE GPUConstantBufferData = {};
    RenderContext->Map(GlobalBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &GPUConstantBufferData);
    memcpy(GPUConstantBufferData.pData, &GlobalConstantBuffer, sizeof(VS_CONSTANT_BUFFER));
    RenderContext->Unmap(GlobalBuffer, 0);
    RenderContext->VSSetConstantBuffers( 0, 1, &GlobalBuffer);
}

void SetWorldMat4(ID3D11DeviceContext *RenderContext, mat4 World)
{
    GlobalConstantBuffer.World = World;
    D3D11_MAPPED_SUBRESOURCE GPUConstantBufferData = {};
    RenderContext->Map(GlobalBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &GPUConstantBufferData);
    memcpy(GPUConstantBufferData.pData, &GlobalConstantBuffer, sizeof(VS_CONSTANT_BUFFER));
    RenderContext->Unmap(GlobalBuffer, 0);
    RenderContext->VSSetConstantBuffers( 0, 1, &GlobalBuffer);
}

void SetColor(ID3D11DeviceContext *RenderContext, float R, float G, float B)
{
    v3 Color = {R, G, B};
    GlobalConstantBuffer.Color = Color;
    D3D11_MAPPED_SUBRESOURCE GPUConstantBufferData = {};
    RenderContext->Map(GlobalBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &GPUConstantBufferData);
    memcpy(GPUConstantBufferData.pData, &GlobalConstantBuffer, sizeof(VS_CONSTANT_BUFFER));
    RenderContext->Unmap(GlobalBuffer, 0);
    RenderContext->VSSetConstantBuffers( 0, 1, &GlobalBuffer); 
}

LRESULT CALLBACK WndProc(HWND   Window,
                         UINT   Message,
                         WPARAM WParam,
                         LPARAM LParam)
{
    LRESULT Result = {};
    switch(Message)
    {
        case WM_CLOSE:
        {
            GlobalRunning = false;
        }break;
        case WM_DESTROY:
        {
            GlobalRunning = false;
        }break;
        default:
        {
           Result = DefWindowProcA(Window, Message, WParam, LParam);
        }break; 
    }
    return Result;
}

void InitializeDirecX11(HWND                   Window,
                        ID3D11Device           **Device,
                        ID3D11DeviceContext    **RenderContext,
                        IDXGISwapChain         **SwapChain,
                        ID3D11RenderTargetView **BackBuffer)
{
    RECT ClientDimensions = {};
    GetClientRect(Window, &ClientDimensions);
    unsigned int Width  = ClientDimensions.right - ClientDimensions.left;
    unsigned int Height = ClientDimensions.bottom - ClientDimensions.top;

    // -1: Define the device types and feature level we want to check for.
     D3D_DRIVER_TYPE DriverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_SOFTWARE
    };
    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    unsigned int DriverTypesCout = ARRAYSIZE(DriverTypes);
    unsigned int FeatureLevelsCount = ARRAYSIZE(FeatureLevels);
    
    // -2: Create the Direct3D device, rendering context, and swap chain.
    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = 1;
    SwapChainDesc.BufferDesc.Width = Width;
    SwapChainDesc.BufferDesc.Height = Height;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.OutputWindow = Window;
    SwapChainDesc.Windowed = true;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
   
    D3D_FEATURE_LEVEL FeatureLevel = {}; 
    D3D_DRIVER_TYPE DriverType = {};
    HRESULT Result = {};
    for(unsigned int Driver = 0;
        Driver < DriverTypesCout;
        ++Driver)
    {
        Result = D3D11CreateDeviceAndSwapChain(NULL, DriverTypes[Driver], NULL, 0,
                                               FeatureLevels, FeatureLevelsCount,
                                               D3D11_SDK_VERSION, &SwapChainDesc, 
                                               SwapChain, Device, &FeatureLevel,
                                               RenderContext);
        if(SUCCEEDED(Result))
        {
            DriverType = DriverTypes[Driver];
            break;
        } 
    }

    // -3: Create render target.
    ID3D11Texture2D *BackBufferTexture = 0;

    Result = (*SwapChain)->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&BackBufferTexture);
    Result = (*Device)->CreateRenderTargetView(BackBufferTexture, 0, BackBuffer);
    if(BackBufferTexture)
    {
        BackBufferTexture->Release();
    }
    (*RenderContext)->OMSetRenderTargets(1, BackBuffer, 0);

    // -4: Set the viewport.
    D3D11_VIEWPORT Viewport;
    Viewport.Width  = (float)Width;
    Viewport.Height = (float)Height;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    (*RenderContext)->RSSetViewports(1, &Viewport);
}

void DrawRect(ID3D11DeviceContext *RenderContext, 
              float X, float Y, float Width, float Height,
              float R, float G, float B)
{    
    mat4 Scale = ScaleMat4({Width, Height, 0.0f});
    mat4 Translate = TranslationMat4({X, Y, 0.0f});
    mat4 World = Translate * Scale;

    SetWorldMat4(RenderContext, World);
    SetColor(RenderContext, R, G, B);

    unsigned int Stride =  sizeof(vertex_pos);
    unsigned int Offset = 0;
    RenderContext->IASetInputLayout(InputLayout);
    RenderContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
    RenderContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    RenderContext->VSSetShader(VertexShader, 0, 0);
    RenderContext->PSSetShader(PixelShader,  0, 0);
    RenderContext->Draw(6, 0);
}

void 
DrawLineBetweenNodes(ID3D11DeviceContext *RenderContext, node *Start, node *End)
{
    for(float T = 0.0f;
        T <= 1.0f;
        T += 0.01f)
    {
        v2 RectPos = LerpV2({Start->XPos, Start->YPos}, 
                            {End->XPos, End->YPos}, T);
        DrawRect(RenderContext, 
                 RectPos.X, RectPos.Y,
                 2.0f, 2.0f,
                 1.0f, 1.0f, 0.5f);
    } 
}

void
DrawLineBetweenNeighboursEx(ID3D11DeviceContext *RenderContext, graph *Graph)
{
    node *FirstNode = Graph->Nodes;
    FirstNode -= (Graph->NodesCount - 1);
    for(int NodeIndex = 0;
        NodeIndex < Graph->NodesCount;
        ++NodeIndex)
    {
        node *ActualNode = FirstNode + NodeIndex;
        
        node_list *ActualNodeList = ActualNode->LastNeighbour;
        while(ActualNodeList)
        {
            node *ActualNeighbour = ActualNodeList->Neighbour;
            DrawLineBetweenNodes(RenderContext, ActualNode, ActualNeighbour);
            ActualNodeList = ActualNodeList->PrevNeighbour;
        }     
    }
}

int WINAPI WinMain(HINSTANCE Instance,
                   HINSTANCE PrevInstance,
                   LPSTR     lpCmdLine,
                   int       nShowCmd)
{
    WNDCLASSEX WindowClass = { 0 };
    WindowClass.cbSize = sizeof( WNDCLASSEX ) ;
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = WndProc;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor( NULL, IDC_ARROW );
    WindowClass.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = "DX11BookWindowClass";

    RegisterClassEx(&WindowClass);

    RECT Rect = { 0, 0, 640, 480 };
    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

    HWND Window = CreateWindowA("DX11BookWindowClass",
                                "Direct3D11 Test",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                Rect.right - Rect.left,
                                Rect.bottom - Rect.top,
                                NULL, NULL, Instance, NULL);
    if(Window)
    {
        
        app_memory AppMemory = {};
        AppMemory.Size = Megabytes(256);
        AppMemory.Memory = VirtualAlloc(0, AppMemory.Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

        arena NodeArena = {};
        arena NeighboursArena = {};
        arena NodeListArena = {};
        InitArena(&AppMemory, &NodeArena, Megabytes(20));
        InitArena(&AppMemory, &NeighboursArena, Megabytes(20));
        InitArena(&AppMemory, &NodeListArena, Megabytes(20));

        // NOTE(manuto): Test Graph
        graph Graph = {};
        node *A = AddNodeToGraph(&Graph,  0.0f, 100.0f, &NodeArena);
        node *B = AddNodeToGraph(&Graph, 100.0f, 200.0f, &NodeArena);
        node *C = AddNodeToGraph(&Graph, 100.0f, 100.0f, &NodeArena);
        node *D = AddNodeToGraph(&Graph, 100.0f, 0.0f, &NodeArena);
        node *E = AddNodeToGraph(&Graph, 200.0f, 100.0f, &NodeArena);
        // LinkList Test...
        AddNodeToList(A, B, &NodeListArena); 
        AddNodeToList(A, D, &NodeListArena); 
        AddNodeToList(B, A, &NodeListArena);
        AddNodeToList(A, C, &NodeListArena);
        AddNodeToList(E, B, &NodeListArena);
        AddNodeToList(B, C, &NodeListArena);
        AddNodeToList(B, E, &NodeListArena);
        AddNodeToList(C, A, &NodeListArena);
        AddNodeToList(E, C, &NodeListArena);
        AddNodeToList(E, D, &NodeListArena);
        AddNodeToList(C, B, &NodeListArena);
        AddNodeToList(C, E, &NodeListArena);
        AddNodeToList(D, A, &NodeListArena);
        AddNodeToList(D, E, &NodeListArena);
        

        
        RECT ClientDimensions = {};
        GetClientRect(Window, &ClientDimensions);
        unsigned int Width  = ClientDimensions.right - ClientDimensions.left;
        unsigned int Height = ClientDimensions.bottom - ClientDimensions.top;

        // Initialize Direct3D11
        // - Define the device types and feature level we want to check for.
        // - Create the Direct3D device, rendering context, and swap chain.
        // - Create render target.
        // - Set the viewport.
        ID3D11Device           *Device        = 0;
        ID3D11DeviceContext    *RenderContext = 0;
        IDXGISwapChain         *SwapChain     = 0;
        ID3D11RenderTargetView *BackBuffer    = 0;
        InitializeDirecX11(Window, &Device, &RenderContext, &SwapChain, &BackBuffer);
        
        // Create a vertex Buffer.
        vertex_pos Vertices[] = 
        {
            { 1.0f,  1.0f, 0.0f, 1.0f, 1.0f},
            { 1.0f,  0.0f, 0.0f, 1.0f, 0.0f},
            { 0.0f,  0.0f, 0.0f, 0.0f, 0.0f},
            { 0.0f,  0.0f, 0.0f, 0.0f, 0.0f},
            { 0.0f,  1.0f, 0.0f, 0.0f, 1.0f},
            { 1.0f,  1.0f, 0.0f, 1.0f, 1.0f},
        };

        // buffer description
        D3D11_BUFFER_DESC VertexDesc = {};
        VertexDesc.Usage = D3D11_USAGE_DEFAULT;
        VertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        VertexDesc.ByteWidth = sizeof(vertex_pos) * 6;
        // pass the buffer data (Vertices).
        D3D11_SUBRESOURCE_DATA ResourceData = {};
        ResourceData.pSysMem = Vertices;
        // Create the VertexBuffer
        HRESULT Result = Device->CreateBuffer(&VertexDesc, &ResourceData, &VertexBuffer);
        
        // Create Vertex and Pixel Shaders:
        // - First we need to compile the shader.
        //   this ones can be compiled from a string or a file,
        //   we use D3DCompile becouse we want to compile from a string,
        //   if you want too compile from a file, you have to use D3DX11CompileFromFile
        ID3DBlob *VertexShaderCompiled = 0;
        ID3DBlob *ErrorVertexShader    = 0;
        Result = D3DCompile((void *)VertexShaderSource,
                            (SIZE_T)StringLength(VertexShaderSource),
                            0, 0, 0, "VS_Main", "vs_4_0",
                            D3DCOMPILE_ENABLE_STRICTNESS, 0,
                            &VertexShaderCompiled, &ErrorVertexShader);
        if(ErrorVertexShader != 0)
        {
            ErrorVertexShader->Release();
        }

        ID3DBlob *PixelShaderCompiled = 0;
        ID3DBlob *ErrorPixelShader    = 0;
        Result = D3DCompile((void *)PixelShaderSource,
                            (SIZE_T)StringLength(PixelShaderSource),
                            0, 0, 0, "PS_Main", "ps_4_0",
                            D3DCOMPILE_ENABLE_STRICTNESS, 0,
                            &PixelShaderCompiled, &ErrorPixelShader);
        if(ErrorPixelShader != 0)
        {
            ErrorPixelShader->Release();
        }

        // Create the Vertex Shader.
        Result = Device->CreateVertexShader(VertexShaderCompiled->GetBufferPointer(),
                                            VertexShaderCompiled->GetBufferSize(), 0,
                                            &VertexShader);
        // Create the Input layout.
        D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
             0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
            0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        unsigned int TotalLayoutElements = ARRAYSIZE(InputLayoutDesc);
        Result = Device->CreateInputLayout(InputLayoutDesc,
                                           TotalLayoutElements,
                                           VertexShaderCompiled->GetBufferPointer(),
                                           VertexShaderCompiled->GetBufferSize(),
                                           &InputLayout);
        // Create Pixel Shader.
        Result = Device->CreatePixelShader(PixelShaderCompiled->GetBufferPointer(),
                                           PixelShaderCompiled->GetBufferSize(), 0,
                                           &PixelShader); 
        VertexShaderCompiled->Release();
        PixelShaderCompiled->Release();
 
        // Create constant Buffers and  
        // seting projection and world Matrix
        D3D11_BUFFER_DESC ConstantBufferDesc;
        ConstantBufferDesc.ByteWidth = sizeof(VS_CONSTANT_BUFFER) + (16 - (sizeof(VS_CONSTANT_BUFFER) % 16));
        ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ConstantBufferDesc.MiscFlags = 0;
        ConstantBufferDesc.StructureByteStride = 0;
        Result = Device->CreateBuffer(&ConstantBufferDesc, 0, &GlobalBuffer);
        
        mat4 Orthogonal = OrthogonalMat4(Width, Height, 1.0f, 100.0f);
        SetProjectionMat4(RenderContext, Orthogonal);
        mat4 World = IdentityMat4();
        SetWorldMat4(RenderContext, World);
        SetColor(RenderContext, 1.0f, 1.0f ,1.0f);


        // Loading a texture
        ID3D11ShaderResourceView *ColorMap;
        ID3D11SamplerState *ColorMapSampler;
        
        // first we create a simple test Texture
        unsigned int Pixels[32*32];
        for(int Y = 0;
            Y < 32;
            ++Y)
        {
            for(int X = 0;
                X < 32;
                ++X)
            {
                Pixels[Y * 32 + X] = 0xFFFFFF00;   
            }
        }

        D3D11_SUBRESOURCE_DATA Data = {};
        Data.pSysMem = (void *)Pixels;
        Data.SysMemPitch = 32*sizeof(unsigned int);
        Data.SysMemSlicePitch = 0;

        D3D11_TEXTURE2D_DESC TextureDesc = {}; 
        TextureDesc.Width = 32;
        TextureDesc.Height = 32;
        TextureDesc.MipLevels = 1;
        TextureDesc.ArraySize = 1;
        TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        TextureDesc.SampleDesc.Count = 1;
        TextureDesc.SampleDesc.Quality = 0;
        TextureDesc.Usage = D3D11_USAGE_DEFAULT;
        TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        TextureDesc.CPUAccessFlags = 0;
        TextureDesc.MiscFlags = 0;
        // Create oout Texture 
        ID3D11Texture2D *Texture;
        Result = Device->CreateTexture2D(&TextureDesc, &Data, &Texture);
        if(SUCCEEDED(Result))
        {
            OutputDebugString("SUCCEEDED Creating Texture\n");
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceDesc = {};
        ShaderResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        ShaderResourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        ShaderResourceDesc.Texture2D.MostDetailedMip = 0;
        ShaderResourceDesc.Texture2D.MipLevels = 1;
        Result = Device->CreateShaderResourceView(Texture, &ShaderResourceDesc, &ColorMap);
        if(SUCCEEDED(Result))
        {
            OutputDebugString("SUCCEEDED Creating Shader resource view\n");
        }

        D3D11_SAMPLER_DESC ColorMapDesc = {};
        ColorMapDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        ColorMapDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        ColorMapDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        ColorMapDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        ColorMapDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; //D3D11_FILTER_MIN_MAG_MIP_LINEAR | D3D11_FILTER_MIN_MAG_MIP_POINT
        ColorMapDesc.MaxLOD = D3D11_FLOAT32_MAX;
        Result = Device->CreateSamplerState(&ColorMapDesc, &ColorMapSampler);
        if(SUCCEEDED(Result))
        {
            OutputDebugString("SUCCEEDED Creating sampler state\n");
        }

        GlobalRunning = true;
        ShowWindow(Window, nShowCmd);
        while(GlobalRunning)
        {
            MSG Message = {};
            while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
            {
                switch(Message.message)
                {
                    default:
                    {
                        TranslateMessage(&Message);
                        DispatchMessage(&Message);
                    }break;
                }
            }
            
            // Update and Render
            float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            RenderContext->ClearRenderTargetView(BackBuffer, ClearColor);
            
            DrawLineBetweenNeighboursEx(RenderContext, &Graph);
            node *FirstNode = Graph.Nodes;
            FirstNode -= (Graph.NodesCount - 1);
            for(int NodeIndex = 0;
                NodeIndex < Graph.NodesCount;
                ++NodeIndex)
            {
                node *ActualNode = FirstNode + NodeIndex; 
                DrawRect(RenderContext, 
                         ActualNode->XPos, ActualNode->YPos,
                         10.0f, 10.0f,
                         1.0f, 0.0f, 0.3f);
            }

            SwapChain->Present(0, 0);
             
        }
        if(BackBuffer) BackBuffer->Release();
        if(SwapChain) SwapChain->Release();
        if(RenderContext) RenderContext->Release();
        if(Device) Device->Release();
         
    }
    return 0;
}



