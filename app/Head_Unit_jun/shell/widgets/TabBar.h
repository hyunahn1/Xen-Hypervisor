/**
 * @file TabBar.h
 * @brief Tab navigation: Media | Call | Navigation | Lighting | Settings
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef TABBAR_H
#define TABBAR_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>

class TabBar : public QWidget
{
    Q_OBJECT

public:
    explicit TabBar(QWidget *parent = nullptr);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

signals:
    void tabSelected(int index);

private slots:
    void onTabClicked();

private:
    void setupUI();
    void updateTabStyles();

    QHBoxLayout *m_layout;
    QVector<QPushButton*> m_tabs;
    int m_currentIndex;
};

#endif // TABBAR_H
