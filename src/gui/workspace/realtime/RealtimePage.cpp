#include "RealtimePage.h"
#include "ui_RealtimePage.h"

RealtimePage::RealtimePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RealtimePage)
{
    ui->setupUi(this);
}

RealtimePage::~RealtimePage()
{
    delete ui;
}
