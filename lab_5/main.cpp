/*

	Copyright 2011 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Tutorial 33 - Instanced Rendering
*/

#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "engine_common.h"
#include "pipeline.h"
#include "camera.h"
#include "texture.h"
#include "lighting_technique.h"
#include "glut_backend.h"
#include "mesh.h"
#ifdef FREETYPE
#include "freetypeGL.h"
#endif

#define WINDOW_WIDTH  1280  
#define WINDOW_HEIGHT 1024

#define NUM_ROWS 50
#define NUM_COLS 20
#define NUM_INSTANCES NUM_ROWS * NUM_COLS


class Tutorial33 : public ICallbacks
{
public:

    Tutorial33()
    {
        m_pGameCamera = NULL;
        m_pEffect = NULL;
        m_scale = 0.0f;
        m_directionalLight.Color = Vector3f(1.0f, 1.0f, 1.0f);
        m_directionalLight.AmbientIntensity = 0.55f;
        m_directionalLight.DiffuseIntensity = 0.9f;
        m_directionalLight.Direction = Vector3f(1.0f, 0.0, 0.0);

        m_persProjInfo.FOV = 60.0f;
        m_persProjInfo.Height = WINDOW_HEIGHT;
        m_persProjInfo.Width = WINDOW_WIDTH;
        m_persProjInfo.zNear = 1.0f;
        m_persProjInfo.zFar = 100.0f;  
        
        m_pMesh = NULL;
        m_frameCount = 0;
        m_fps = 0.0f;
    }

    ~Tutorial33()
    {
        SAFE_DELETE(m_pEffect);
        SAFE_DELETE(m_pGameCamera);
        SAFE_DELETE(m_pMesh);
    }    

    bool Init()
    {
        Vector3f Pos(7.0f, 3.0f, 0.0f);
        Vector3f Target(0.0f, -0.2f, 1.0f);
        Vector3f Up(0.0, 1.0f, 0.0f);

        m_pGameCamera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT, Pos, Target, Up);
      
        m_pEffect = new LightingTechnique();

        if (!m_pEffect->Init()) {
            printf("Error initializing the lighting technique\n");
            return false;
        }

        m_pEffect->Enable();

        m_pEffect->SetColorTextureUnit(COLOR_TEXTURE_UNIT_INDEX);
        m_pEffect->SetDirectionalLight(m_directionalLight);
        m_pEffect->SetMatSpecularIntensity(0.0f);
        m_pEffect->SetMatSpecularPower(0);
        m_pEffect->SetColor(0, Vector4f(1.0f, 0.5f, 0.5f, 0.0f));
        m_pEffect->SetColor(1, Vector4f(0.5f, 1.0f, 1.0f, 0.0f));
        m_pEffect->SetColor(2, Vector4f(1.0f, 0.5f, 1.0f, 0.0f));
        m_pEffect->SetColor(3, Vector4f(1.0f, 1.0f, 1.0f, 0.0f));

        m_pMesh = new Mesh();

        if (!m_pMesh->LoadMesh("../Content/spider.obj")) {
            return false;            
        }
        
#ifdef FREETYPE
        if (!m_fontRenderer.InitFontRenderer()) {
            return false;
        }
#endif
        
        m_time = glutGet(GLUT_ELAPSED_TIME);

        CalcPositions();
        
        return true;
    }

    void Run()
    {
        GLUTBackendRun(this);
    }
    

    virtual void RenderSceneCB()
    {   
        CalcFPS();
        
        m_scale += 0.005f;

        m_pGameCamera->OnRender();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_pEffect->Enable();
        m_pEffect->SetEyeWorldPos(m_pGameCamera->GetPos());
        
        Pipeline p;
        p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());
        p.SetPerspectiveProj(m_persProjInfo);   
        p.Rotate(0.0f, 90.0f, 0.0f);
        p.Scale(0.005f, 0.005f, 0.005f);                

        Matrix4f WVPMatrics[NUM_INSTANCES];
        Matrix4f WorldMatrices[NUM_INSTANCES];
        
        for (unsigned int i = 0 ; i < NUM_INSTANCES ; i++) {
            Vector3f Pos(m_positions[i]);
            Pos.y += sinf(m_scale) * m_velocity[i];
            p.WorldPos(Pos);        
            WVPMatrics[i] = p.GetWVPTrans().Transpose();
            WorldMatrices[i] = p.GetWorldTrans().Transpose();
        }
        
        m_pMesh->Render(NUM_INSTANCES, WVPMatrics, WorldMatrices);
        
        RenderFPS();
        
        glutSwapBuffers();
    }

    virtual void IdleCB()
    {
        RenderSceneCB();
    }

    virtual void SpecialKeyboardCB(int Key, int x, int y)
    {
        m_pGameCamera->OnKeyboard(Key);
    }


    virtual void KeyboardCB(unsigned char Key, int x, int y)
    {
        switch (Key) {
            case 'q':
                glutLeaveMainLoop();
                break;
        }
    }


    virtual void PassiveMouseCB(int x, int y)
    {
        m_pGameCamera->OnMouse(x, y);
    }
    
    
    virtual void MouseCB(int Button, int State, int x, int y)
    {
    }


private:
    
    void CalcFPS()
    {
        m_frameCount++;
        
        int time = glutGet( GLUT_ELAPSED_TIME );

        if (time - m_time > 1000) {
            m_fps = (float)m_frameCount * 1000.0f / (time - m_time);
            m_time = time;
            m_frameCount = 0;
        }
    }
    
    
    void RenderFPS()
    {
        char text[32];
        SNPRINTF(text, sizeof(text), "FPS: %.2f", m_fps);
#ifdef FREETYPE
        m_fontRenderer.RenderText(10, 10, text);        
#endif
    }
    
    
    void CalcPositions()
    {
        for (unsigned int i = 0; i < NUM_ROWS ; i++) {
            for (unsigned int j = 0 ; j < NUM_COLS ; j++) {
                unsigned int Index = i * NUM_COLS + j;
                m_positions[Index].x = (float)j;
                m_positions[Index].y = RandomFloat() * 5.0f;
                m_positions[Index].z = (float)i;
                m_velocity[Index] = RandomFloat();
                if (i & 1) {
                    m_velocity[Index] *= (-1.0f);
                }
            }
        }                   
    }

    LightingTechnique* m_pEffect;
    Camera* m_pGameCamera;
    float m_scale;
    DirectionalLight m_directionalLight;
    Mesh* m_pMesh;
    PersProjInfo m_persProjInfo;
#ifdef FREETYPE
    FontRenderer m_fontRenderer;
#endif
    int m_time;
    int m_frameCount;
    float m_fps;    
    Vector3f m_positions[NUM_INSTANCES];            
    float m_velocity[NUM_INSTANCES];
};


int main(int argc, char** argv)
{
    Magick::InitializeMagick(*argv);
    GLUTBackendInit(argc, argv);

    if (!GLUTBackendCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 32, false, "Tutorial 33")) {
        return 1;
    }
    
    SRANDOM;

    Tutorial33* pApp = new Tutorial33();

    if (!pApp->Init()) {
        return 1;
    }
    
    pApp->Run();

    delete pApp;
 
    return 0;
}