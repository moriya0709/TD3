#include "Model.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

void Model::Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename) {
	// 引数で受け取ってメンバ変数に記録する
	modelCommon_ = modelCommon;
	dxCommon_ = dxCommon;

	// モデル読み込み
	modelData = LoadModelFile(directoryPath, filename);

	GenerateOutlineNormal(modelData.vertices);

	// *頂点データ* //

	// リソース
	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	// バッファリソース
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);
	// 書き込む
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	// *マテリアル* //

	// リソース
	materialResource = dxCommon_->CreateBufferResource(sizeof(Material));
	// 書き込む為のアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 初期値を書き込む
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = true;
	materialData->enableToonShading = true;
	materialData->uvTransform = MakeIdentity4x4();
	// ★ mtlから読んだ自己発光カラーを代入！
	materialData->emissive = modelData.material.emissive;

	// ★ 追加：shininessの初期化忘れを防ぐ（適当な光沢具合を入れる）
	materialData->shininess = 70.0f;

	materialData->fresnelColor = { 1.0f, 1.0f, 1.0f, 0.5f };
	materialData->fresnelPower = 4.0f;
	materialData->rimColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->rimThreshold = 0.5f;

	// *テクスチャ* //

	// 読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	// 番号取得
	modelData.material.textureIndex = TextureManager::GetInstance()->GetSrvIndex(modelData.material.textureFilePath);

}

void Model::Draw() {

	// RootSignatureを設定。PSOに設定しているけど別途設定が必要
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定

	// マテリアルCBufferの場所を設定
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureFilePath));
	
	// 描画
	dxCommon_->GetCommandList()->DrawInstanced(static_cast<UINT>(modelData.vertices.size()), 1, 0, 0);

}

// .mtlファイルの読み込み
//MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
//	MaterialData materialData; // 構築するMaterialData
//	std::string line; // ファイルから読んだ１行を格納するもの
//	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
//	assert(file.is_open()); // とりあえず聞けなかったら止める
//
//	while (std::getline(file, line)) {
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier;
//
//		// 拡散テクスチャ
//		if (identifier == "map_Kd") {
//			std::string textureFilename;
//			s >> textureFilename;
//			materialData.textureFilePath =
//				directoryPath + "/" + textureFilename;
//		}
//
//		// エミッシブカラー
//		else if (identifier == "Ke") {
//			s >> materialData.emissive.x
//				>> materialData.emissive.y
//				>> materialData.emissive.z;
//		}
//
//
//	}
//
//	return materialData;
//}

Node Model::ReadNode(aiNode* node)
{
	Node result;
	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);//assimpの行列からSRTを抽出する関数を利用
	result.transform.scale = { scale.x,scale.y,scale.z };//scaleはそのまま
	result.transform.rotate = { rotate.x,-rotate.y,-rotate.z,rotate.w };//X軸を反転、さらに回転方向が逆なので軸を反転させる
	result.transform.translate = { -translate.x,translate.y,translate.z };//X軸を反転
	result.localMatrix = MakeAffineMatrixQuaternion(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name = node->mName.C_Str();//Node名を格納
	result.children.resize(node->mNumChildren);//子供の数だけ確保
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
	{
		//再帰的に読んで階層構造体を作っていく
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}

	return result;
}

// .objファイルの読み込み
ModelData Model::LoadModelFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData; // 構築するModelData
	//std::vector<Vector4> positions; //位置
	//std::vector<Vector3> normals; // 法線
	//std::vector<Vector2> texcoords; //　テクスチャ座標
	//std::string line; // ファイルから読んだ1行を格納するもの

	//assimpでobjを読む
	Assimp::Importer importer;
	std::string filePath(directoryPath + "/" + filename); // ファイルを開く
	//assert(file.is_open()); // とりあえず開けなかったら止める

	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_Triangulate);
	assert(scene->HasMeshes());//メッシュがないのは対応しない

	modelData.rootNode = ReadNode(scene->mRootNode);

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)//mesh解析
	{
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());//法線がないMeshは今回は非対応
		assert(mesh->HasTextureCoords(0));//TexcoordがないMeshは今回は非対応

		// 現在の頂点数を保存しておく（複数メッシュ対応のため）
		uint32_t vertexOffset = static_cast<uint32_t>(modelData.vertices.size());

		//まず頂点をすべて解析して配列に突っ込む
		for (uint32_t v = 0; v < mesh->mNumVertices; ++v)
		{
			aiVector3D& position = mesh->mVertices[v];
			aiVector3D& normal = mesh->mNormals[v];
			aiVector3D& texcoord = mesh->mTextureCoords[0][v];

			VertexData vertex;
			vertex.position = { position.x,position.y,position.z,1.0f };
			vertex.normal = { normal.x,normal.y,normal.z };
			vertex.texcoord = { texcoord.x,texcoord.y };
			//aiProcess_MakeLeftHandedはz*=-1で右手->左手に変換するので手動で対処
			vertex.position.x *= -1.0f;
			vertex.normal.x *= -1.0f;
			modelData.vertices.push_back(vertex);
		}

		//面インデックスを解析して配列に突っ込む
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)//face解析
		{
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);//三角形のみサポート

			for (uint32_t element = 0; element < face.mNumIndices; ++element)//vertex解析
			{
				uint32_t vertexIndex = face.mIndices[element];
				//オフセットを足してインデックスを保存する
				modelData.indices.push_back(vertexIndex + vertexOffset);
			}
		}
	}
	//material解析
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
	{
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0)
		{
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
		}
	}


	//while (std::getline(file, line)) {
	//	std::string identifier;
	//	std::istringstream s(line);
	//	s >> identifier; // 先頭の識別子を読む
	//
	//	// identifierに応じた処理
	//	if (identifier == "v") {
	//		Vector4 position;
	//		s >> position.x >> position.y >> position.z;
	//		position.w = 1.0f;
	//		position.x *= -1.0f;
	//		positions.push_back(position);
	//	} else if (identifier == "vt") {
	//		Vector2 texcoord;
	//		s >> texcoord.x >> texcoord.y;
	//		texcoords.push_back(texcoord);
	//	} else if (identifier == "vn") {
	//		Vector3 normal;
	//		s >> normal.x >> normal.y >> normal.z;
	//		normal.x *= -1.0f;
	//		normals.push_back(normal);
	//	} else if (identifier == "f") {
	//		VertexData triangle[3];
	//		// 面は三角形限定。その他は未対応
	//		for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
	//			std::string vertexDefinition;
	//			s >> vertexDefinition;
	//			// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
	//			std::istringstream v(vertexDefinition);
	//			uint32_t elementIndices[3];
	//			for (int32_t element = 0; element < 3; ++element) {
	//				std::string index;
	//				std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
	//				elementIndices[element] = std::stoi(index);
	//			}
	//			// 要素へのIndexから、実際の要素の値を取得して、頂点を構成する
	//			Vector4 position = positions[elementIndices[0] - 1];
	//			Vector2 texcoord = texcoords[elementIndices[1] - 1];
	//			Vector3 normal = normals[elementIndices[2] - 1];
	//			texcoord.y = 1.0f - texcoord.y;
	//			triangle[faceVertex] = { position,texcoord,normal };
	//		}
	//
	//		// 頂点を逆順で登録することで、回り順を逆にする
	//		modelData.vertices.push_back(triangle[2]);
	//		modelData.vertices.push_back(triangle[1]);
	//		modelData.vertices.push_back(triangle[0]);
	//	} else if (identifier == "mtllib") {
	//		// materialTemplateLibraryファイルの名前を取得する
	//		std::string materialFilename;
	//		s >> materialFilename;
	//		// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
	//		modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
	//	}
	//}

	return modelData;
}

void Model::GenerateOutlineNormal(std::vector<VertexData>& vertices) {
	const float epsilon = 0.0001f;

	for (size_t i = 0; i < vertices.size(); ++i) {

		Vector3 sumNormal = { 0,0,0 };

		for (size_t j = 0; j < vertices.size(); ++j) {

			// 座標がほぼ同じなら同一頂点とみなす
			if (fabs(vertices[i].position.x - vertices[j].position.x) < epsilon &&
				fabs(vertices[i].position.y - vertices[j].position.y) < epsilon &&
				fabs(vertices[i].position.z - vertices[j].position.z) < epsilon) {
				sumNormal += vertices[j].normal;
			}
		}

		vertices[i].outlineNormal = Normalize(sumNormal);
	}
}
