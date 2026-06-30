#include "EmoteWheel.h"
#include <QApplication>
#include <QPixmap>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    EmoteWheel w;
    w.addEmote("haha", QPixmap(":/icons/haha.png"));
    w.addEmote("wow", QPixmap(":/icons/wow.png"));
    w.addEmote("sad", QPixmap(":/icons/sad.png"));
    w.addEmote("angry", QPixmap(":/icons/angry.png"));
    w.addEmote("love", QPixmap(":/icons/love.png"));
    w.addEmote("like", QPixmap(":/icons/like.png"));

    QObject::connect(&w, &EmoteWheel::emoteSelected, [](const QString &name) {
        qDebug() << "Emote selected:" << name;
    });

    w.show();
    return QApplication::exec();
}