#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QtCore/QModelIndex>
#include <QtGui/QWidget>

namespace vpvl {
class Bone;
class Face;
class PMDModel;
}

namespace internal {
class TimelineTableModel;
}

class QTableView;

class TimelineWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget *parent = 0);
    ~TimelineWidget();

public slots:
    void registerBone(vpvl::Bone *bone);
    void registerFace(vpvl::Face *face);
    void setModel(vpvl::PMDModel *value);
    void selectCell(QModelIndex modelIndex);

signals:
    void boneDidSelect(vpvl::Bone *bone);

private:
    QTableView *m_tableView;
    internal::TimelineTableModel *m_tableModel;
    vpvl::PMDModel *m_selectedModel;
};

#endif // TIMLINEWIDGET_H
