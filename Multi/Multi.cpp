#include "DXUT.h"
#include <fstream>
#include <sstream>

// ------------------------------------------------------------------------------

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj =
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };

	uint seraSelecionada = 0;
};

// ------------------------------------------------------------------------------

class Multi : public App{
private:
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pipelineState = nullptr;
    ID3D12PipelineState* pipelineStateLinhas = nullptr;
    vector<Object> scene;

    Timer timer;
    bool spinning = true;

    XMFLOAT4X4 Identity = {};
    XMFLOAT4X4 View = {};
    XMFLOAT4X4 Proj = {};


	// Matrizes para as 4 viewports
    XMFLOAT4X4 um = {};
	XMFLOAT4X4 dois = {};
	XMFLOAT4X4 tres = {};

	XMFLOAT4X4 ortogonal = {};

    int selecionada  = 0;
    bool telatoda = false;


    float theta = 0;
    float phi = 0;
    float radius = 0;
    float lastMousePosX = 0;
    float lastMousePosY = 0;


    vector <D3D12_VIEWPORT> viewports;
    Mesh linhas;

public:
    void Init();
    void Update();
    void Draw();
    void Finalize();

    void BuildRootSignature();
    void BuildPipelineState();
	void CarregarObjetos(const std::string& filePath, Object& obj, XMFLOAT4 cor);
};
// ------------------------------------------------------------------------------
 void Multi::CarregarObjetos(const std::string& filePath, Object& obj, XMFLOAT4 cor) {
     std::ifstream file(filePath);

	 std::vector<Vertex> positions;
     std::vector<uint> indices;

     std::string line;
     while (std::getline(file, line)) {
         std::istringstream iss(line);
         std::string prefix;
         iss >> prefix;

         if (prefix == "v") {
             Vertex position;
             iss >> position.pos.x >> position.pos.y >> position.pos.z;
			 position.color = cor;
             positions.push_back(position);
         }
         else if (prefix == "f") {
             uint posIndex[4]{};
             string js;
             uint indice = 0;
             while (!iss.eof()) {
				 iss >> posIndex[indice] >> js;
                 indice += 1;
             }

             indices.push_back(posIndex[0] - 1);
             indices.push_back(posIndex[1] - 1);
             indices.push_back(posIndex[2] - 1);

             // Se houver um quarto vértice, adiciona os índices para o segundo triângulo
             if (indice == 4) {
                 indices.push_back(posIndex[0] - 1);
                 indices.push_back(posIndex[2] - 1);
                 indices.push_back(posIndex[3] - 1);
             }

         }
     }
	 graphics->ResetCommands();

	 obj.mesh = new Mesh();
	 Mesh& meshData = *obj.mesh;

     meshData.VertexBuffer(positions.data(), positions.size() * sizeof(Vertex), sizeof(Vertex));
	 meshData.IndexBuffer(indices.data(), indices.size() * sizeof(uint), DXGI_FORMAT_R32_UINT);
	 meshData.ConstantBuffer(sizeof(ObjectConstants), 4);
	 meshData.CopyConstants(new ObjectConstants);
	 obj.submesh.indexCount = indices.size();


	 graphics->SubmitCommands();
 }
// ------------------------------------------------------------------------------

void Multi::Init()
{

    
    // Obter o tamanho da janela
    float width = (float)window->Width();
    float height = (float)window->Height();






    // Definir os viewports
    viewports = {
        { 0.0f, 0.0f, width / 2, height / 2, 0.0f, 1.0f }, // Top-Left
        { width / 2, 0.0f, width / 2, height / 2, 0.0f, 1.0f }, // Top-Right
        { 0.0f, height / 2, width / 2, height / 2, 0.0f, 1.0f }, // Bottom-Left
        { width / 2, height / 2, width / 2, height / 2, 0.0f, 1.0f } // Bottom-Right
    };


	// Inicializa o objeto gráfico

    XMStoreFloat4x4(&Proj, XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.0f),
        window->AspectRatio(),
        1.0f, 100.0f));

    XMStoreFloat4x4(&ortogonal, XMMatrixOrthographicLH(
		5*window->AspectRatio(), 5, 1.0f, 110.0f));

    XMStoreFloat4x4(&um, XMMatrixLookAtLH(
		XMVectorSet(0.0f, 1.5f, -5.0f, 1.0f), //como o z ta negativo, estamos vendo de frente kk
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

    XMStoreFloat4x4(&dois, XMMatrixLookAtLH(
        XMVectorSet(0.0f, +5.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), //olhando para baixo
        XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)));

    XMStoreFloat4x4(&tres, XMMatrixLookAtLH(
		XMVectorSet(-5.0f, 1.5f, 0.0f, 1.0f), //olhando de lado e de por positivo olha do oturo lado kkk
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));











    graphics->ResetCommands();

    // -----------------------------
    // Parâmetros Iniciais da Câmera
    // -----------------------------
 
    // controla rotação da câmera
    theta = XM_PIDIV4;
    phi = 1.3f;
    radius = 5.0f;

    // pega última posição do mouse
    lastMousePosX = (float) input->MouseX();
    lastMousePosY = (float) input->MouseY();

    // inicializa as matrizes Identity e View para a identidade
    Identity = View = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };

    // inicializa a matriz de projeção
    XMStoreFloat4x4(&Proj, XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.0f), 
        window->AspectRatio(), 
        1.0f, 100.0f));

    // ----------------------------------------
    // Criação da Geometria: Vértices e Índices
    // ----------------------------------------

    Box box(2.0f, 2.0f, 2.0f);
    Cylinder cylinder(1.0f, 0.5f, 3.0f, 20, 20);
    Sphere sphere(1.0f, 20, 20);
    Grid grid(3.0f, 3.0f, 20, 20);

    for (auto& v : box.vertices) v.color = XMFLOAT4(DirectX::Colors::Orange);
    for (auto& v : cylinder.vertices) v.color = XMFLOAT4(DirectX::Colors::Yellow);
    for (auto& v : sphere.vertices) v.color = XMFLOAT4(DirectX::Colors::Crimson);
    for (auto& v : grid.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);

    // ---------------------------------------------------------------
    // Alocação e Cópia de Vertex, Index e Constant Buffers para a GPU
    // ---------------------------------------------------------------

    // box
    Object boxObj;
    XMStoreFloat4x4(&boxObj.world,
        XMMatrixScaling(0.4f, 0.4f, 0.4f) *
        XMMatrixTranslation(-1.0f, 0.41f, 1.0f));

    boxObj.mesh = new Mesh();
    boxObj.mesh->VertexBuffer(box.VertexData(), box.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    boxObj.mesh->IndexBuffer(box.IndexData(), box.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    boxObj.mesh->ConstantBuffer(sizeof(ObjectConstants),4);
    boxObj.submesh.indexCount = box.IndexCount();
    scene.push_back(boxObj);

    // cylinder
    Object cylinderObj;
    XMStoreFloat4x4(&cylinderObj.world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        XMMatrixTranslation(1.0f, 0.75f, -1.0f));

    cylinderObj.mesh = new Mesh();
    cylinderObj.mesh->VertexBuffer(cylinder.VertexData(), cylinder.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    cylinderObj.mesh->IndexBuffer(cylinder.IndexData(), cylinder.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    cylinderObj.mesh->ConstantBuffer(sizeof(ObjectConstants),4);
    cylinderObj.submesh.indexCount = cylinder.IndexCount();
    scene.push_back(cylinderObj);

    // sphere
    Object sphereObj;
    XMStoreFloat4x4(&sphereObj.world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        XMMatrixTranslation(0.0f, 0.5f, 0.0f));

    sphereObj.mesh = new Mesh();
    sphereObj.mesh->VertexBuffer(sphere.VertexData(), sphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    sphereObj.mesh->IndexBuffer(sphere.IndexData(), sphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    sphereObj.mesh->ConstantBuffer(sizeof(ObjectConstants),4);
    sphereObj.submesh.indexCount = sphere.IndexCount();
    scene.push_back(sphereObj);

    // grid
    Object gridObj;
    gridObj.mesh = new Mesh();
    gridObj.world = Identity;
    gridObj.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    gridObj.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    gridObj.mesh->ConstantBuffer(sizeof(ObjectConstants),4);
    gridObj.submesh.indexCount = grid.IndexCount();
    scene.push_back(gridObj);
 


    Vertex lineVertices[] =
    {
        // Linha vertical no meio
        { XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        // Linha horizontal no meio
        { XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
    };

    linhas.VertexBuffer(lineVertices, sizeof(lineVertices), sizeof(Vertex));
    linhas.ConstantBuffer(sizeof(ObjectConstants));
    linhas.CopyConstants(new ObjectConstants );

    // ---------------------------------------

    BuildRootSignature();
    BuildPipelineState();    

    // ---------------------------------------
    graphics->SubmitCommands();

    timer.Start();
}

// ------------------------------------------------------------------------------

void Multi::Update(){

    if (input->KeyPress('1')) {
        Object meshO;
        CarregarObjetos("ball.obj", meshO, XMFLOAT4(Colors::Chocolate));
        scene.push_back(meshO);
    }
    if (input->KeyPress('2')) {
        Object meshO;
        CarregarObjetos("capsule.obj", meshO, XMFLOAT4(Colors::ForestGreen));
        scene.push_back(meshO);
    }
    if (input->KeyPress('3')) {
        Object meshO;
        CarregarObjetos("house.obj", meshO, XMFLOAT4(Colors::Red));
        scene.push_back(meshO);
    }
    if (input->KeyPress('4')) {
        Object meshO;
        CarregarObjetos("monkey.obj", meshO, XMFLOAT4(Colors::AliceBlue));
        scene.push_back(meshO);
    }
    if (input->KeyPress('5')) {
        Object meshO;
        CarregarObjetos("thorus.obj", meshO, XMFLOAT4(Colors::PaleGoldenrod));
        scene.push_back(meshO);
    }
	if (input->KeyPress(VK_DELETE)) {
		if (scene.size() > 0) {
			delete scene.at(selecionada).mesh;
            scene.erase(scene.begin() + selecionada);
            if (selecionada >= scene.size()) {
                selecionada = 0;
            }
		}
	}

    if (input->KeyPress(VK_TAB)) {
        if (!scene.empty()) {

            selecionada += 1;
            if (selecionada >= scene.size()) {

                selecionada = 0;
            }

        
		}
	}


    if (input->KeyPress('V')) {
        if (telatoda) {
            telatoda = false;
        }
        else {
			telatoda = true;

        }
	}
	graphics->ResetCommands();

    if (input->KeyPress('B')) {
        Box box(2.0f, 2.0f, 2.0f);
        Object boxObj;
        XMStoreFloat4x4(&boxObj.world,
            XMMatrixScaling(0.4f, 0.4f, 0.4f) *
            XMMatrixTranslation(0.0f, 0.4f, 0.0f));

        boxObj.mesh = new Mesh();
        boxObj.mesh->VertexBuffer(box.VertexData(), box.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        boxObj.mesh->IndexBuffer(box.IndexData(), box.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        boxObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        boxObj.submesh.indexCount = box.IndexCount();
        scene.push_back(boxObj);
    }
	if (input->KeyPress('C')) {
		Cylinder cylinder(1.0f, 0.5f, 3.0f, 20, 20);
		Object cylinderObj;
		XMStoreFloat4x4(&cylinderObj.world,
			XMMatrixScaling(0.5f, 0.5f, 0.5f)*
			XMMatrixTranslation(0.0f, 0.7f, 0.0f)
        );

		cylinderObj.mesh = new Mesh();
		cylinderObj.mesh->VertexBuffer(cylinder.VertexData(), cylinder.VertexCount() * sizeof(Vertex), sizeof(Vertex));
		cylinderObj.mesh->IndexBuffer(cylinder.IndexData(), cylinder.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
		cylinderObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
		cylinderObj.submesh.indexCount = cylinder.IndexCount();
		scene.push_back(cylinderObj);
	}

	if (input->KeyPress('S')) {
		Sphere sphere(1.0f, 20, 20);
		Object sphereObj;
		XMStoreFloat4x4(&sphereObj.world,
			XMMatrixScaling(0.5f, 0.5f, 0.5f) *
			XMMatrixTranslation(0.0f, 0.5f, 0.0f));

		sphereObj.mesh = new Mesh();
		sphereObj.mesh->VertexBuffer(sphere.VertexData(), sphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
		sphereObj.mesh->IndexBuffer(sphere.IndexData(), sphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
		sphereObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
		sphereObj.submesh.indexCount = sphere.IndexCount();
		scene.push_back(sphereObj);
	}
    if (input->KeyPress('G')) {
        GeoSphere geoSphere(1.0f, 3);
        Object geoSphereObj;
        geoSphereObj.mesh = new Mesh();
        geoSphereObj.world = Identity;
        geoSphereObj.mesh->VertexBuffer(geoSphere.VertexData(), geoSphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        geoSphereObj.mesh->IndexBuffer(geoSphere.IndexData(), geoSphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        geoSphereObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        geoSphereObj.submesh.indexCount = geoSphere.IndexCount();
        scene.push_back(geoSphereObj);
    }

	if (input->KeyPress('P')) {
		Grid grid(3.0f, 3.0f, 20, 20);
		Object gridObj;
		gridObj.mesh = new Mesh();
		gridObj.world = Identity;
		gridObj.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
		gridObj.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
		gridObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
		gridObj.submesh.indexCount = grid.IndexCount();
		scene.push_back(gridObj);
	}

    if (input->KeyPress('Q')) {
		Quad quad(2.0f, 2.0f);
		Object quadObj;
		quadObj.mesh = new Mesh();
		quadObj.world = Identity;
		quadObj.mesh->VertexBuffer(quad.VertexData(), quad.VertexCount() * sizeof(Vertex), sizeof(Vertex));
		quadObj.mesh->IndexBuffer(quad.IndexData(), quad.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
		quadObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
		quadObj.submesh.indexCount = quad.IndexCount();
		scene.push_back(quadObj);

    }



	graphics->SubmitCommands();

    if (input->KeyPress(102)) {//6
		XMFLOAT4X4 leste = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixTranslation(0.1f, 0.0f, 0.0f) * XMLoadFloat4x4(&leste));
    }
	if (input->KeyPress(100)) {//4
		XMFLOAT4X4 oeste = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixTranslation(-0.1f, 0.0f, 0.0f)* XMLoadFloat4x4(&oeste));
	}
	if (input->KeyPress(104)) {//8
		XMFLOAT4X4 norte = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixTranslation(0.0f, 0.0f, 0.1f)* XMLoadFloat4x4(&norte));
	}
	if (input->KeyPress(98)) {//2
		XMFLOAT4X4 sul = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixTranslation(0.0f, 0.0f, -0.1f) * XMLoadFloat4x4(&sul));
	}
	if (input->KeyPress(101)) {//5
		XMFLOAT4X4 cima = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixTranslation(0.0f, 0.1f, 0.0f)* XMLoadFloat4x4(&cima));
	}
	if (input->KeyPress(96)) {//0
		XMFLOAT4X4 baixo = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixTranslation(0.0f, -0.1f, 0.0f)* XMLoadFloat4x4(&baixo));
	}
    if (input->KeyPress(33)) {//+
		XMFLOAT4X4 escala = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixScaling(1.1f, 1.1f, 1.1f)* XMLoadFloat4x4(&escala));
    }
	if (input->KeyPress(34)) {//-
		XMFLOAT4X4 escala = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixScaling(0.9f, 0.9f, 0.9f)* XMLoadFloat4x4(&escala));
	}
	if (input->KeyPress(107)) {//+
		XMFLOAT4X4 rotacao = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixRotationY(0.1f)* XMLoadFloat4x4(&rotacao));
	}
	if (input->KeyPress(109)) {//-
		XMFLOAT4X4 rotacao = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixRotationY(-0.1f)* XMLoadFloat4x4(&rotacao));
	}
	if (input->KeyPress(111)) {///
		XMFLOAT4X4 rotacao = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixRotationX(0.1f)* XMLoadFloat4x4(&rotacao));
	}
	if (input->KeyPress(106)) {//*
		XMFLOAT4X4 rotacao = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixRotationX(-0.1f)* XMLoadFloat4x4(&rotacao));
	}
	if (input->KeyPress(105)) {//9
		XMFLOAT4X4 rotacao = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixRotationZ(0.1f)* XMLoadFloat4x4(&rotacao));
	}
	if (input->KeyPress(97)) {//1
		XMFLOAT4X4 rotacao = scene[selecionada].world;
		XMStoreFloat4x4(&scene[selecionada].world, XMMatrixRotationZ(-0.1f)* XMLoadFloat4x4(&rotacao));
	}
	if (input->KeyPress(35)) {//1 do num
		XMFLOAT4X4 identidade = Identity;
		XMStoreFloat4x4(&scene[selecionada].world, XMLoadFloat4x4(&identidade));
	}



    // sai com o pressionamento da tecla ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    // ativa ou desativa o giro do objeto
    if (input->KeyPress('S'))
    {
        spinning = !spinning;

        if (spinning)
            timer.Start();
        else
            timer.Stop();
    }

    float mousePosX = (float)input->MouseX();
    float mousePosY = (float)input->MouseY();

    if (input->KeyDown(VK_LBUTTON))
    {
        // cada pixel corresponde a 1/4 de grau
        float dx = XMConvertToRadians(0.25f * (mousePosX - lastMousePosX));
        float dy = XMConvertToRadians(0.25f * (mousePosY - lastMousePosY));

        // atualiza ângulos com base no deslocamento do mouse 
        // para orbitar a câmera ao redor da caixa
        theta += dx;
        phi += dy;

        // restringe o ângulo de phi ]0-180[ graus
        phi = phi < 0.1f ? 0.1f : (phi > (XM_PI - 0.1f) ? XM_PI - 0.1f : phi);
    }
    else if (input->KeyDown(VK_RBUTTON))
    {
        // cada pixel corresponde a 0.05 unidades
        float dx = 0.05f * (mousePosX - lastMousePosX);
        float dy = 0.05f * (mousePosY - lastMousePosY);

        // atualiza o raio da câmera com base no deslocamento do mouse 
        radius += dx - dy;

        // restringe o raio (3 a 15 unidades)
        radius = radius < 3.0f ? 3.0f : (radius > 15.0f ? 15.0f : radius);
    }

    lastMousePosX = mousePosX;
    lastMousePosY = mousePosY;

    // converte coordenadas esféricas para cartesianas
    float x = radius * sinf(phi) * cosf(theta);
    float z = radius * sinf(phi) * sinf(theta);
    float y = radius * cosf(phi);

    // constrói a matriz da câmera (view matrix)
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&View, view);

    // carrega matriz de projeção em uma XMMATRIX
    XMMATRIX proj = XMLoadFloat4x4(&Proj);

    // modifica matriz de mundo da esfera
    /*
    XMStoreFloat4x4(&scene[2].world,
        XMMatrixScaling(0.5f, 0.5f, 0.5f) *
        XMMatrixRotationY(float(timer.Elapsed())) *
        XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    */
    // ajusta o buffer constante de cada objeto
    int i = 0;
    for (auto & obj : scene){
       
        bool holofote = false;
		if (i == selecionada) {
			holofote = true;
		}
        i++;
        // carrega matriz de mundo em uma XMMATRIX
        XMMATRIX world = XMLoadFloat4x4(&obj.world);      

        // constrói matriz combinada (world x view x proj)
        XMMATRIX WorldViewProj = world * view * proj;        

        // atualiza o buffer constante com a matriz combinada
        ObjectConstants constants;
		constants.seraSelecionada = holofote;
        XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProj));
        obj.mesh->CopyConstants(&constants, 0);

        
		//proj = XMLoadFloat4x4(&ortogonal);

        //WorldViewProj = world * XMLoadFloat4x4(&um) * proj;


		WorldViewProj = world * XMLoadFloat4x4(&um) * XMLoadFloat4x4(&ortogonal);

        ObjectConstants constants2;
		constants2.seraSelecionada = holofote;

        XMStoreFloat4x4(&constants2.WorldViewProj, XMMatrixTranspose(WorldViewProj));
        obj.mesh->CopyConstants(&constants2, 1);


        WorldViewProj = world * XMLoadFloat4x4(&dois) * XMLoadFloat4x4(&ortogonal);

        ObjectConstants constants3;
		constants3.seraSelecionada = holofote;

        XMStoreFloat4x4(&constants3.WorldViewProj, XMMatrixTranspose(WorldViewProj));
        obj.mesh->CopyConstants(&constants3, 2);

        WorldViewProj = world * XMLoadFloat4x4(&tres) * XMLoadFloat4x4(&ortogonal);

        ObjectConstants constants4;
		constants4.seraSelecionada = holofote;

        XMStoreFloat4x4(&constants4.WorldViewProj, XMMatrixTranspose(WorldViewProj));
        obj.mesh->CopyConstants(&constants4, 3);
    }
}

// ------------------------------------------------------------------------------

void Multi::Draw()
{
    // limpa o backbuffer
    graphics->Clear(pipelineStateLinhas); // nhame nhame linhas papilanas


	if (telatoda) {
		graphics->CommandList()->SetPipelineState(pipelineState);
        graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
        ID3D12DescriptorHeap* descriptorHeap = linhas.ConstantBufferHeap();
        graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeap);


        for (auto& obj : scene){
            // comandos de configuração do pipeline
            ID3D12DescriptorHeap* descriptorHeap = obj.mesh->ConstantBufferHeap();
            graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeap);
            graphics->CommandList()->IASetVertexBuffers(0, 1, obj.mesh->VertexBufferView());
            graphics->CommandList()->IASetIndexBuffer(obj.mesh->IndexBufferView());
            graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // ajusta o buffer constante associado ao vertex shader
            graphics->CommandList()->SetGraphicsRootDescriptorTable(0, obj.mesh->ConstantBufferHandle(0));

            // desenha objeto
            graphics->CommandList()->DrawIndexedInstanced(
                obj.submesh.indexCount, 1,
                obj.submesh.startIndex,
                obj.submesh.baseVertex,
                0);
        }
	}
	else {
        // desenha linhas
        graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
        ID3D12DescriptorHeap* descriptorHeap = linhas.ConstantBufferHeap();
        graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeap);
        graphics->CommandList()->SetGraphicsRootDescriptorTable(0, linhas.ConstantBufferHandle(0));
        graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        graphics->CommandList()->IASetVertexBuffers(0, 1, linhas.VertexBufferView());
        graphics->CommandList()->DrawInstanced(4, 1, 0, 0);
        graphics->CommandList()->SetPipelineState(pipelineState); // nhumm voltou a ser triangulo

        //desenha as viewport
        for (int i = 0; i < 4; i++) {
            graphics->CommandList()->RSSetViewports(1, &viewports[i]);

            // desenha objetos da cena
            for (auto& obj : scene)
            {
                // comandos de configuração do pipeline
                ID3D12DescriptorHeap* descriptorHeap = obj.mesh->ConstantBufferHeap();
                graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeap);
                graphics->CommandList()->IASetVertexBuffers(0, 1, obj.mesh->VertexBufferView());
                graphics->CommandList()->IASetIndexBuffer(obj.mesh->IndexBufferView());
                graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                // ajusta o buffer constante associado ao vertex shader
                graphics->CommandList()->SetGraphicsRootDescriptorTable(0, obj.mesh->ConstantBufferHandle(i));

                // desenha objeto
                graphics->CommandList()->DrawIndexedInstanced(
                    obj.submesh.indexCount, 1,
                    obj.submesh.startIndex,
                    obj.submesh.baseVertex,
                    0);
            }
        }
	}
	


    // apresenta o backbuffer na tela
    graphics->Present();    
}

// ------------------------------------------------------------------------------

void Multi::Finalize()
{
    rootSignature->Release();
    pipelineState->Release();

    for (auto& obj : scene)
        delete obj.mesh;
}


// ------------------------------------------------------------------------------
//                                     D3D                                      
// ------------------------------------------------------------------------------

void Multi::BuildRootSignature()
{
    // cria uma única tabela de descritores de CBVs
    D3D12_DESCRIPTOR_RANGE cbvTable = {};
    cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvTable.NumDescriptors = 1;
    cbvTable.BaseShaderRegister = 0;
    cbvTable.RegisterSpace = 0;
    cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // define parâmetro raiz com uma tabela
    D3D12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &cbvTable;

    // uma assinatura raiz é um vetor de parâmetros raiz
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // serializa assinatura raiz
    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* error = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &error));

    if (error != nullptr)
    {
        OutputDebugString((char*)error->GetBufferPointer());
    }

    // cria uma assinatura raiz com um único slot que aponta para  
    // uma faixa de descritores consistindo de um único buffer constante
    ThrowIfFailed(graphics->Device()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

// ------------------------------------------------------------------------------

void Multi::BuildPipelineState()
{
    // --------------------
    // --- Input Layout ---
    // --------------------
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // --------------------
    // ----- Shaders ------
    // --------------------

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
    D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

    // --------------------
    // ---- Rasterizer ----
    // --------------------

    D3D12_RASTERIZER_DESC rasterizer = {};
    //rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_BACK;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // ---------------------
    // --- Color Blender ---
    // ---------------------

    D3D12_BLEND_DESC blender = {};
    blender.AlphaToCoverageEnable = FALSE;
    blender.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // ---------------------
    // --- Depth Stencil ---
    // ---------------------

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencil.FrontFace = defaultStencilOp;
    depthStencil.BackFace = defaultStencilOp;
    
    // -----------------------------------
    // --- Pipeline State Object (PSO) ---
    // -----------------------------------

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = rootSignature;
    pso.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    pso.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    pso.BlendState = blender;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState = rasterizer;
    pso.DepthStencilState = depthStencil;
    pso.InputLayout = { inputLayout, 2 };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = graphics->Antialiasing();
    pso.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));
	
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineStateLinhas));

    vertexShader->Release();
    pixelShader->Release();
}


// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    try
    {
        // cria motor e configura a janela
        Engine* engine = new Engine();
        engine->window->Mode(WINDOWED);
        engine->window->Size(1024, 720);
        engine->window->Color(25, 25, 25);
        engine->window->Title("Multii");
        engine->window->Icon(IDI_ICON);
        engine->window->Cursor(IDC_CURSOR);
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        // cria e executa a aplicação
        engine->Start(new Multi());

        // finaliza execução
        delete engine;
    }
    catch (Error & e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Multi", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------

