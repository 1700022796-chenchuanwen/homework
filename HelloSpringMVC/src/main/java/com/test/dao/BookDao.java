package com.test.dao;

import java.util.List;
import com.test.dao.Book;

public interface BookDao {
	
	 /**
     * ͨ��ID��ѯ����ͼ��
     * 
     * @param id
     * @return
     */
    Book queryById(long id);

    /**
     * ��ѯ����ͼ��
     * 
     * @param offset ��ѯ��ʼλ��
     * @param limit ��ѯ����
     * @return
     */
    List<Book> queryAll( int offset,  int limit);

    /**
     * ���ٹݲ�����
     * 
     * @param bookId
     * @return ���Ӱ����������>1����ʾ���µļ�¼����
     */
    int reduceNumber(long bookId);
}
