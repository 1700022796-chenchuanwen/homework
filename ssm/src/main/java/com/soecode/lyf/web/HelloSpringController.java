package com.soecode.lyf.web;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.servlet.ModelAndView;


import com.soecode.lyf.dao.BookDao;
import com.soecode.lyf.entity.Book;
import org.springframework.beans.factory.annotation.Autowired;

@Controller
public class HelloSpringController {
	
    @Autowired
    private BookDao bookDao;
    
    String message = "Welcome to Spring MVC!!!";
 
    @RequestMapping("/hello")
    public ModelAndView showMessage(@RequestParam(value = "name", required = false, defaultValue = "Spring") String name) {
 
        long bookId = 1000;
        Book book = bookDao.queryById(bookId);
    	
    	
    	String bookname = book.getname();
    	//String bookname = "test";
    	System.out.print(book);
        ModelAndView mv = new ModelAndView("hellospring");//指定视图
        //向视图中添加所要展示或使用的内容，将在页面中使用
        mv.addObject("message", message);
        mv.addObject("name", name);
        mv.addObject("bookname", bookname);
        return mv;
    }
}