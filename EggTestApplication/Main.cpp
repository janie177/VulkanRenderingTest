#include <filesystem>
#include <memory>
#include <glm/glm/glm.hpp>

#include "EggRenderer.h"
#include "InputQueue.h"
#include "Timer.h"


/*
 * Program entry point.
 */
int main()
{
    using namespace egg;

    RendererSettings settings;
    settings.debugFlags = DebugPrintFlags::ERROR | DebugPrintFlags::WARNING;
    settings.vSync = true;
    settings.clearColor = glm::vec4(0.f, 0.5f, 0.9f, 1.f);
    settings.lockCursor = true;
    settings.m_SwapBufferCount = 3;
    settings.shadersPath = std::filesystem::current_path().parent_path().string() + "/Build/shaders/";

    auto renderer = EggRenderer::CreateInstance(settings);
    Camera camera;
    camera.UpdateProjection(70.f, 0.1f, 600.f, static_cast<float>(settings.resolutionX) / static_cast<float>(settings.resolutionY));

    if (renderer->Init(settings))
    {
        std::shared_ptr<EggMesh> sphereMesh;
        std::shared_ptr<EggMesh> planeMesh;
        {
            //Create a sphere mesh.
            Transform meshTransform;
            ShapeCreateInfo shapeInfo;
            shapeInfo.m_Sphere.m_SectorCount = 20;
            shapeInfo.m_Sphere.m_StackCount = 20;
            shapeInfo.m_ShapeType = Shape::SPHERE;
            shapeInfo.m_InitialTransform = meshTransform.GetTransformation();
            sphereMesh = renderer->CreateMesh(shapeInfo);
        }
        {
            //Create a plane mesh.
            Transform meshTransform;
            meshTransform.Translate({ 0.f, -1.f, 0.f });
            ShapeCreateInfo shapeInfo;
            shapeInfo.m_Radius = 100.f;
            shapeInfo.m_ShapeType = Shape::PLANE;
            shapeInfo.m_InitialTransform = meshTransform.GetTransformation();
            planeMesh = renderer->CreateMesh(shapeInfo);
        }
    	
        //Sphere instances
        constexpr auto NUM_CUBE_INSTANCES = 500;
        std::vector<MeshInstance> meshInstances(NUM_CUBE_INSTANCES);
        Transform t;
        for(int i = 0; i < NUM_CUBE_INSTANCES; ++i)
        {
            t.Translate(t.GetForward() * 0.2f);
            t.Translate(t.GetUp() * 0.2f);
            t.RotateAround({ 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, 0.1f);
            meshInstances[i].m_Transform = t.GetTransformation();
        }

    	//Plane instance (default constructed)
        std::vector<MeshInstance> planeInstances;
        planeInstances.resize(1);

    	//Create materials.
        MaterialCreateInfo materialInfo;
        materialInfo.m_MetallicFactor = 0.5f;
        materialInfo.m_RoughnessFactor = 0.3f;
        materialInfo.m_AlbedoFactor = { 1.f, 0.f, 0.f };
        auto material = renderer->CreateMaterial(materialInfo);
        materialInfo.m_AlbedoFactor = { 1.f, 1.f, 1.f };
        materialInfo.m_MetallicFactor = 0.f;
        materialInfo.m_RoughnessFactor = 1.f;
        auto planeMaterial = renderer->CreateMaterial(materialInfo);

        //Draw information and draw calls.
        DrawData drawData;
        auto planeDrawCall = renderer->CreateDynamicDrawCall(planeMesh, planeInstances, { planeMaterial });
        auto drawCall = renderer->CreateDynamicDrawCall(sphereMesh, meshInstances, {material});

    	//Build it all into a draw data object.
        drawData.SetCamera(camera).AddDrawCall(planeDrawCall).AddDrawCall(drawCall);

        //Time FPS etc.
        Timer timer;

        //Main loop
        static int frameIndex = 0;
        bool run = true;
        while(run)
        {
            timer.Reset();
            ++frameIndex;

            //Randomly change material color once in a while.
            if(frameIndex % 100 == 0)
            {
                const float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
				const float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
				const float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                material->SetAlbedoFactor({ r, g, b });
            }

            //Draw
            drawData.SetCamera(camera); //Update camera.
            run = renderer->DrawFrame(drawData);

            //Update input
            auto input = renderer->QueryInput();
            MouseEvent mEvent;
            KeyboardEvent kEvent;
            while(input.GetNextEvent(mEvent))
            {
                constexpr float mouseDivider = 400.f;
                if(mEvent.action == MouseAction::SCROLL)
                {
                    
                }
                else if(mEvent.action == MouseAction::MOVE_X)
                {
                    camera.GetTransform().Rotate(Transform::GetWorldUp(), static_cast<float>(mEvent.value) / -mouseDivider);
                }
                else if(mEvent.action == MouseAction::MOVE_Y)
                {
                    camera.GetTransform().Rotate(camera.GetTransform().GetRight(), static_cast<float>(mEvent.value) / -mouseDivider);
                }
                else if(mEvent.action == MouseAction::CLICK)
                {
                    std::string mbutton = (mEvent.button == MouseButton::MMB ? "MMB" : mEvent.button == MouseButton::RMB ? "RMB" : "LMB");
                    printf("Mouse button clicked: %s.\n", mbutton.c_str());
                }
            }

            constexpr float movementSpeed = 0.01f;
            const auto forwardState = input.GetKeyState(EGG_KEY_W);
            const auto rightState = input.GetKeyState(EGG_KEY_D);
            const auto leftState = input.GetKeyState(EGG_KEY_A);
            const auto backwardState = input.GetKeyState(EGG_KEY_S);
            const auto upState = input.GetKeyState(EGG_KEY_E);
            const auto downState = input.GetKeyState(EGG_KEY_Q);
            if(forwardState != ButtonState::NOT_PRESSED) camera.GetTransform().Translate(camera.GetTransform().GetForward() * -movementSpeed);
            if (rightState != ButtonState::NOT_PRESSED) camera.GetTransform().Translate(camera.GetTransform().GetRight() * movementSpeed);
            if (leftState != ButtonState::NOT_PRESSED) camera.GetTransform().Translate(camera.GetTransform().GetLeft() * movementSpeed);
            if (backwardState != ButtonState::NOT_PRESSED) camera.GetTransform().Translate(camera.GetTransform().GetBack() * -movementSpeed);
            if (upState != ButtonState::NOT_PRESSED) camera.GetTransform().Translate(camera.GetTransform().GetWorldUp() * movementSpeed);
            if (downState != ButtonState::NOT_PRESSED) camera.GetTransform().Translate(camera.GetTransform().GetWorldDown() * movementSpeed);

            while(input.GetNextEvent(kEvent))
            {
                if(kEvent.action == KeyboardAction::KEY_PRESSED)
                {
                    printf("Key pressed: %u.\n", kEvent.keyCode);

                    //Stop the program when escape is pressed.
                    if(kEvent.keyCode == EGG_KEY_ESCAPE)
                    {
                        run = false;
                    }

                    if(kEvent.keyCode == EGG_KEY_ENTER)
                    {
                        renderer->Resize(!renderer->IsFullScreen(), 1280, 720);
                        const auto resolution = renderer->GetResolution();
                        camera.UpdateProjection(70.f, 0.1f, 1000.f, resolution.x / resolution.y);
                    }
                }
            }

            if (frameIndex % 1 == 0)
            {
                printf("Frame time: %f ms.\n", timer.Measure(TimeUnit::MILLIS));
                printf("Frame #%i.\n", frameIndex);
            }
        }
    }
    else
    {
        printf("Could not init renderer.\n");
    }

    printf("Done running renderer.\n");

    //Delete all allocated objects.
    if(renderer->CleanUp())
    {
        printf("Renderer successfully cleaned up!\n");
    }
    else
    {
        printf("Could not clean up renderer properly!\n");
    }

    printf("Program execution finished.\nPress any key to continue.\n");
}