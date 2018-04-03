package com.soecode.lyf.dao;

import java.util.List;

import com.soecode.lyf.entity.Herb;

public interface HerbDao {

    /**
     * ͨ
     * 
     * @param id
     * @return
     */
    Herb queryById(long id);

    Herb queryByName(String name);
    /**
     * 
     * 
     * @param offset 
     * @param limit 
     * @return
     */
    List<Herb> queryAll( int offset,  int limit);



}