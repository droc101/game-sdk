//
// Created by droc101 on 2/17/26.
//

#include "ModelEditor.h"
#include <format>
#include <game_sdk/SDKWindow.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/ConvexHull.h>
#include <libassets/type/StaticCollisionMesh.h>
#include <libassets/util/Error.h>
#include <string>
#include <utility>

void ModelEditor::DestroyExistingModel()
{
    if (!modelLoaded)
    {
        return;
    }
    modelLoaded = false;
}

void ModelEditor::ImportLod(const std::string &path)
{
    ModelAsset model = modelViewer.GetModel();
    if (!model.AddLod(path))
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to import model LOD!"));
        return;
    }
    DestroyExistingModel();
    modelViewer.SetModel(std::move(model));
    modelLoaded = true;
}

void ModelEditor::ImportSingleHull(const std::string &path)
{
    ModelAsset model = modelViewer.GetModel();
    Error::ErrorCode e = Error::ErrorCode::UNKNOWN;
    const ConvexHull hull = ConvexHull(path, e);
    if (e != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage("Failed to import collision hull");
        return;
    }
    model.AddHull(hull);
    DestroyExistingModel();
    modelViewer.SetModel(std::move(model));
    modelLoaded = true;
}

void ModelEditor::ImportMultipleHulls(const std::string &path)
{
    ModelAsset model = modelViewer.GetModel();
    if (model.AddHulls(path) != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage("Failed to import collision hulls");
        return;
    }
    DestroyExistingModel();
    modelViewer.SetModel(std::move(model));
    modelLoaded = true;
}

void ModelEditor::ImportStaticCollider(const std::string &path)
{
    ModelAsset model = modelViewer.GetModel();
    Error::ErrorCode e = Error::ErrorCode::UNKNOWN;
    model.SetStaticCollisionMesh(StaticCollisionMesh(path, e));
    if (e != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage("Failed to import static collision model");
        return;
    }
    DestroyExistingModel();
    modelViewer.SetModel(std::move(model));
    modelLoaded = true;
}
