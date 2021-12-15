#ifndef CANVAS2D_H
#define CANVAS2D_H

#include <memory>
#include <concepts>

#include "SupportCanvas2D.h"
#include "../scenegraph/RayScene.h"

class CS123SceneCameraData;

/**
 * @class Canvas2D
 *
 * 2D canvas that students will implement in the Brush and Filter assignments. The same canvas
 * will be used to display raytraced images in the Intersect and Ray assignments.
 */
class Canvas2D : public SupportCanvas2D {
    Q_OBJECT
public:
    Canvas2D();
    virtual ~Canvas2D();

    void setScene(RayScene *scene);

    // UI will call this from the button on the "Ray" dock
    void renderImage(CS123SceneCameraData* camera, int width, int height);

    void renderSphere( int width, int height);
    void rendertree( int width, int height);
    void rendermandelbulb( int width, int height);
    void renderepicscene1( int width, int height);
    void renderepicscene2( int width, int height);
    void renderepicscene3( int width, int height);

    // This will be called when the settings have changed
    virtual void settingsChanged();

    QImage* getImage() { return m_image; }

    std::size_t Width = 0;
    std::size_t Height = 0;

    auto operator[](std::integral auto y) {
        return reinterpret_cast<RGBA*>(m_image->bits() + y * m_image->bytesPerLine());
    }

public slots:
    // UI will call this from the button on the "Ray" dock
    void cancelRender();

    // UI will call this from the button on the "Filter" dock
    void filterImage();

protected:
    virtual void paintEvent(QPaintEvent *);  // Overridden from SupportCanvas2D.
    virtual void mouseDown(int x, int y);    // Called when left mouse button is pressed on canvas
    virtual void mouseDragged(int x, int y); // Called when left mouse button is dragged on canvas
    virtual void mouseUp(int x, int y);      // Called when left mouse button is released

    // Called when the size of the canvas has been changed
    virtual void notifySizeChanged(int w, int h);



private:

    std::unique_ptr<RayScene> m_rayScene = {};

    //TODO: [BRUSH, INTERSECT, RAY] Put your member variables here.

};

#endif // CANVAS2D_H
