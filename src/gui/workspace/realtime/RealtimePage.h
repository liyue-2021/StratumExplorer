#ifndef REALTIMEPAGE_H
#define REALTIMEPAGE_H

#include <QWidget>

namespace Ui {
    class RealtimePage;
}

class RealtimePage : public QWidget
{
    Q_OBJECT

public:
    explicit RealtimePage(QWidget *parent = nullptr);
    ~RealtimePage();

private:
    Ui::RealtimePage *ui;
};

#endif // REALTIMEPAGE_H
