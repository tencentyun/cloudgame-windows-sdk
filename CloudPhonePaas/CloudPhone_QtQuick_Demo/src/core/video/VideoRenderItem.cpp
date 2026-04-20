#include "VideoRenderItem.h"

#include <QOpenGLContext>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGNode>
#include <QSGRendererInterface>
#include <QSGTexture>
#include <QThread>

#include "YuvNode.h"
#include "YuvTestPattern.h"

VideoRenderItem::VideoRenderItem(QQuickItem* parent) : QQuickItem(parent) { setFlag(ItemHasContents, true); }

VideoRenderItem::~VideoRenderItem() {}

void VideoRenderItem::setFrame(VideoFrameDataPtr frame) {
  bool wasEmpty = !hasFrame();
  m_frame = frame;
  m_frameDirty = true;
  if (wasEmpty && hasFrame()) {
    emit firstFrameArrived();
  }
  update();
}

bool VideoRenderItem::hasFrame() const {
  return m_frame && m_frame->width > 0 && m_frame->height > 0 && m_frame->data_y != nullptr &&
         m_frame->data_u != nullptr && m_frame->data_v != nullptr;
}

QSGNode* VideoRenderItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
  if (!hasFrame()) {
    delete oldNode;
    return nullptr;
  }

  YuvNode* node = dynamic_cast<YuvNode*>(oldNode);
  if (!node) {
    delete oldNode;
    node = new YuvNode();
  }

  node->setFrame(window(), m_frame.data(), QSizeF(width(), height()), m_frameDirty);
  m_frameDirty = false;
  return node;
}