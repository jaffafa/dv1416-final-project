#include "LightManager.h"


LightManager::LightManager(void)
{
	// Should be same as in Light.fx
	int POINTLIGHTS			= 10;
	int DIRECTIONALLIGHTS	= 2;

	PointLight pointLight;
	pointLight.Ambient		= XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	pointLight.Diffuse		= XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pointLight.Specular		= XMFLOAT4(0.0f, 0.0f, 0.0f, 4.0f);
	pointLight.Position		= XMFLOAT3(0.0f, 0.0f, 0.0f);
	pointLight.Range		= -1.0f;
	pointLight.Att			= XMFLOAT3(0.0f, 0.1f, 0.0f);
	pointLight.Pad			= 0;

	for(int i = 0; i < POINTLIGHTS; i++)
	{
		m_PLights.push_back(pointLight);
		}

	DirectionalLight directionalLight;
	directionalLight.Ambient	= XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	directionalLight.Diffuse	= XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	directionalLight.Specular	= XMFLOAT4(1.0f, 1.0f, 1.0f, 4.0f);
	directionalLight.Direction	= XMFLOAT3(0.0f, -1.0f, 0.0f);
	directionalLight.Pad		= 1;
	for(int i = 0; i < DIRECTIONALLIGHTS; i++)
	{
		m_DLights.push_back(directionalLight);
	}

	m_light = NULL;
	m_state = Add;
	MouseDown = false;
}

LightManager::~LightManager(void)
{
}

void LightManager::init(HWND hWnd, ID3D11Device* device, Camera* camera)
{
	m_hWnd			= hWnd;
	m_camera		= camera;
	D3DX11CreateShaderResourceViewFromFile(device, "Content/img/light.png", 0, 0, &m_texture, 0 );
}

PointLight* LightManager::AddLight( XMFLOAT3 position, LightType type )
{
	for(int i = 0; i < m_PLights.size(); i++)
	{
		if (m_PLights[i].Range < 0)
		{
			m_PLights[i].Position	= position;
			m_PLights[i].Range		= 500;
			return &m_PLights[i];
		}
	}
	return NULL;
}
void LightManager::RemoveLight( PointLight* light )
{
	light->Position		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	light->Range		= -1.f;
}
void LightManager::MoveLightY( PointLight* light, const Ray& ray )
{
	light->Position.y = computePlaneIntersection(light->Position.x, XMFLOAT3(1,0,0), ray).y;
}
void LightManager::MoveLightXZ( PointLight* light, const Ray& ray )
{
	light->Position = computePlaneIntersection(light->Position.y, XMFLOAT3(0,1,0), ray);
}
void LightManager::ClearLights()
{
	for(int i = 0; i < m_PLights.size(); i++)
	{
		RemoveLight(&m_PLights[i]);
	}
}

std::vector<PointLight> LightManager::getPLights()
{
	return m_PLights;
}

std::vector<DirectionalLight> LightManager::getDLights()
{
	return m_DLights;
}

void LightManager::update(float dt)
{
	if (m_light != NULL)
	{
		GUI::PointLightOptions::getInstance().show(true);
	}
	else
		GUI::PointLightOptions::getInstance().show(false);

	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{
		POINT cursorPosition;
		if (GetCursorPos(&cursorPosition))
		{
			ScreenToClient(m_hWnd, &cursorPosition);
			Ray ray = m_camera->computeRay(cursorPosition);

			switch (m_state)
			{
			case Add:
				if (!MouseDown)
					m_light = AddLight(computePlaneIntersection(0.f, XMFLOAT3(0,1,0), ray), POINT_LIGHT);
				MoveLightY(m_light, ray);
				break;
			case Remove:
				m_light = computeIntersection(ray);
				if (m_light != NULL)
					RemoveLight(m_light);
				break;
			case MoveXZ:
				if (!MouseDown)
					m_light = computeIntersection(ray);
				if (m_light != NULL)
				{
					if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
						MoveLightY(m_light, ray);
					else
						MoveLightXZ(m_light, ray);
				}
				break;
			case MoveY:
				if (!MouseDown)
					m_light = computeIntersection(ray);
				if (m_light != NULL)
				{
					if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
						MoveLightXZ(m_light, ray);
					else
						MoveLightY(m_light, ray);
				}
				break;
			}
		}
		MouseDown = true;
	}
	else
		MouseDown = false;
}



void LightManager::render(ID3D11DeviceContext* deviceContext, Shader* shader, const Camera& camera)
{
	struct data {
		PointLight* light;
		float order;
		data(PointLight* p, XMFLOAT3 cam)
		{
			light = p;
			XMVECTOR diff = XMLoadFloat3(&p->Position) - XMLoadFloat3(&cam);
			order = XMVectorGetX(XMVector3Dot(diff, diff));
		}
		bool operator<(const data& a) const
		{
			return order > a.order;
		}
	};

	std::vector<data> drawList;
	for(int i = 0; i < m_PLights.size(); i++)
	{
		if (m_PLights[i].Range > 0.f)
			drawList.push_back(data(&m_PLights[i], camera.getPosition()));
	}
	std::sort(drawList.begin(), drawList.end());

	XMMATRIX world			= XMMatrixIdentity();
	XMMATRIX worldViewProj  = world * camera.getViewProj();
	XMFLOAT3 cameraPosition = camera.getPosition();
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	for (int i = 0; i < drawList.size(); i++) 
	{
		if (drawList[i].light == m_light)
			shader->setBool("gSelected", true);
		else
			shader->setBool("gSelected", false);

		shader->setMatrix("gWorld", world);
		shader->setMatrix("gWorldViewProj", worldViewProj);
		shader->setFloat3("gCameraPosition", cameraPosition);
		shader->setRawData("gLight", drawList[i].light, sizeof(PointLight));
		shader->setResource("gDiffuseMap", m_texture);

		shader->Apply();
		deviceContext->Draw(1, 0);
	}
}

PointLight* LightManager::computeIntersection(const Ray& ray)
{
	PointLight* tempLight;
	float distance = -1;
	float light_r = 2.f;

	// Check for the closest light intersecting the ray.
	for(int i = 0; i < m_PLights.size(); i++)
	{
		if (m_PLights[i].Range > 0.f)
		{
		XMVECTOR light_pos = XMLoadFloat3(&m_PLights[i].Position);
		XMVECTOR l = light_pos - ray.origin;	
		float l2 = XMVectorGetX(XMVector3Dot(l, ray.direction));
		if (l2 > 0)
		{
			float l_sq = XMVectorGetX(XMVector3Dot(l, l));;
			float r_sq = light_r*light_r;
			if (l_sq > r_sq)
			{
				if(l_sq-l2*l2 <= r_sq) 
				{
					float b = XMVectorGetX(XMVector3Dot(ray.direction, (ray.origin - light_pos)));
					float c = XMVectorGetX(XMVector3Dot((ray.origin - light_pos), (ray.origin - light_pos)))-r_sq;
					float h = b*b-c;
					if (h > 0)
					{
						float t1 = -b + sqrtf(h);
						float t2 = -b - sqrtf(h);
						float t = t1 < t2 ? t1 : t2;
						if (t > 0)
						{
							if ((distance > 0 && distance > t) || distance < 0)
							{
								distance = t;
								tempLight = &m_PLights[i];
							}
						}
					}
				}
			}
		}
		}
	}
	if (distance > 0)
		return tempLight;
	else
		return NULL;
}
XMFLOAT3	LightManager::computePlaneIntersection(float Y, XMFLOAT3 normal, const Ray& ray)
{
	XMVECTOR pnormal = XMLoadFloat3(&normal);
	float t = (Y - XMVectorGetX(XMVector3Dot(ray.origin, pnormal))) / XMVectorGetX(XMVector3Dot(ray.direction, pnormal));
	XMFLOAT3 m_targetPosition;
	XMStoreFloat3(&m_targetPosition, ray.origin + t * ray.direction);
	return m_targetPosition;
}

void LightManager::onEvent(const std::string& sender, const std::string& eventName)
{
	if (sender == "PointLight Options")
	{
		GUI::PointLightOptions& pointlightOptions = GUI::PointLightOptions::getInstance();
		if (m_light != NULL)
		{
			if (eventName == "Ambient Range")
				m_light->Range = (float)pointlightOptions.getTrackbarValue(eventName);
			else if (eventName == "Ambient R")
				m_light->Ambient.x = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;
			else if (eventName == "Ambient G")
				m_light->Ambient.y = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;
			else if (eventName == "Ambient B")
				m_light->Ambient.z = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;

			else if (eventName == "Linear Modifier")
				m_light->Att.y = (float)pointlightOptions.getTrackbarValue(eventName) / 100;
			else if (eventName == "Diffuse R")
				m_light->Diffuse.x = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;
			else if (eventName == "Diffuse G")
				m_light->Diffuse.y = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;
			else if (eventName == "Diffuse B")
				m_light->Diffuse.z = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;

			else if (eventName == "Specular Power")
				m_light->Specular.w = (float)pointlightOptions.getTrackbarValue(eventName) / 100;
			else if (eventName == "Specular R")
				m_light->Specular.x = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;
			else if (eventName == "Specular G")
				m_light->Specular.y = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;
			else if (eventName == "Specular B")
				m_light->Specular.z = (float)pointlightOptions.getTrackbarValue(eventName) / 1000;
		}
	}
	if (sender == "DirectionalLight Options")
	{
		GUI::DirectionalLightOptions& directionallightOptions = GUI::DirectionalLightOptions::getInstance();
		if (eventName == "OFF / ON / SHADOW")
		{
			int lightnr = (int)directionallightOptions.getTrackbarValue("Directional Light") - 1;
			m_DLights[lightnr].Pad = (float)directionallightOptions.getTrackbarValue(eventName);
		}
		else if (eventName == "X")
		{
			int lightnr = (int)directionallightOptions.getTrackbarValue("Directional Light") - 1;
			m_DLights[lightnr].Direction.x = ((float)directionallightOptions.getTrackbarValue(eventName) - 100)/10;
		}
		else if (eventName == "Z")
		{
			int lightnr = (int)directionallightOptions.getTrackbarValue("Directional Light") - 1;
			m_DLights[lightnr].Direction.z = ((float)directionallightOptions.getTrackbarValue(eventName) - 100)/10;
		}
	}
}