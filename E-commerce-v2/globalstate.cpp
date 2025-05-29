#include "globalstate.h"

GlobalState::GlobalState(QObject *parent) : QObject(parent) {}

QString GlobalState::username() const {
    return m_username;
}

QString GlobalState::userType() const {
    return m_userType;
}

bool GlobalState::isConsumer() const {
    return m_isConsumer;
}

bool GlobalState::isMerchant() const {
    return m_isMerchant;
}

double GlobalState::balance() const {
    return m_balance;
}

void GlobalState::setUsername(const QString &username) {
    if (m_username != username) {
        m_username = username;
        emit usernameChanged();
    }
}

void GlobalState::setUserType(const QString &userType) {
    if (m_userType != userType) {
        m_userType = userType;
        emit userTypeChanged();
    }
}

void GlobalState::setIsConsumer(bool isConsumer) {
    if (m_isConsumer != isConsumer) {
        m_isConsumer = isConsumer;
        emit isConsumerChanged();
    }
}

void GlobalState::setIsMerchant(bool isMerchant) {
    if (m_isMerchant != isMerchant) {
        m_isMerchant = isMerchant;
        emit isMerchantChanged();
    }
}

void GlobalState::setBalance(double balance) {
    if(qFuzzyCompare(m_balance, balance)) {
        return ;
    }

    m_balance = balance;
    emit balanceChanged();
}
